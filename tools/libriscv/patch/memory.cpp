#include "machine.hpp"

#include "decoder_cache.hpp"
#include <inttypes.h>
#ifdef RISCV_BINARY_TRANSLATION
#include <dlfcn.h> // Linux-only
#endif
#ifdef __linux__
#define DEMANGLE_ENABLED
#include <sys/mman.h>
extern "C" char *
__cxa_demangle(const char *name, char *buf, size_t *n, int *status);
#endif

namespace riscv
{
	template <int W>
	Memory<W>::Memory(Machine<W>& mach, std::string_view bin,
					MachineOptions<W> options)
		: m_machine{mach},
		  m_original_machine {true},
		  m_binary {bin}
	{
		if (options.page_fault_handler != nullptr)
		{
			this->m_page_fault_handler = std::move(options.page_fault_handler);
		}
		else if (options.memory_max != 0)
		{
			const address_t pages_max = options.memory_max / Page::size();
			assert(pages_max >= 1);

			if (options.use_memory_arena)
			{
#ifdef __linux__
				const size_t len = pages_max * Page::size();
				this->m_arena = (PageData *)mmap(NULL, len, PROT_READ | PROT_WRITE,
					MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
				this->m_arena_pages = pages_max;
				// mmap() returns MAP_FAILED (-1) when mapping fails
				if (UNLIKELY(this->m_arena == MAP_FAILED)) {
					this->m_arena = nullptr;
					this->m_arena_pages = 0;
				}
#else
				// TODO: XXX: Investigate if this is a time sink
				this->m_arena = new PageData[pages_max];
				this->m_arena_pages = pages_max;
#endif
			}

			this->m_page_fault_handler =
			[pages_max] (auto& mem, const address_t page, bool init) -> Page&
			{
				if (mem.pages_active() < pages_max)
				{
					// Within linear arena at the start
					if (page < mem.m_arena_pages)
					{
						const PageAttributes attr {
							.read  = true,
							.write = true,
							.non_owning = true
						};
						return mem.allocate_page(page, attr, &mem.m_arena[page]);
					}
					// Create page on-demand
					return mem.allocate_page(page,
						init ? PageData::INITIALIZED : PageData::UNINITIALIZED);
				}
				throw MachineException(OUT_OF_MEMORY, "Out of memory", pages_max);
			};
		} else {
			throw MachineException(OUT_OF_MEMORY, "Max memory was zero", 0);
		}
		if (!m_binary.empty()) {
			// Add a zero-page at the start of address space
			this->initial_paging();
			// load ELF binary into virtual memory
			this->binary_loader(options);
		}
	}
	template <int W>
	Memory<W>::Memory(Machine<W>& mach, const Machine<W>& other, MachineOptions<W> options)
	  : m_machine{mach},
		m_original_machine {false},
		m_binary{other.memory.binary()}
	{
		this->machine_loader(other, options);
	}

	template <int W>
	Memory<W>::~Memory()
	{
		this->clear_all_pages();
		// only the original machine owns rodata range
		if (!this->m_original_machine) {
			m_ropages.pages.release();
		}
#ifdef RISCV_BINARY_TRANSLATION
		if (m_bintr_dl)
			dlclose(m_bintr_dl);
#endif
		if (this->m_arena != nullptr) {
#ifdef __linux__
			munmap(this->m_arena, this->m_arena_pages * Page::size());
#else
			delete[] this->m_arena;
#endif
		}
	}

	template <int W> RISCV_INTERNAL
	void Memory<W>::reset()
	{
		// Hard to support because of things like
		// serialization, machine options and machine forks
	}

	template <int W>
	void Memory<W>::clear_all_pages()
	{
		this->m_pages.clear();
		this->invalidate_reset_cache();
	}

	template <int W> RISCV_INTERNAL
	void Memory<W>::initial_paging()
	{
		if (m_pages.find(0) == m_pages.end()) {
			// add a guard page to catch zero-page accesses
			install_shared_page(0, Page::guard_page());
		}
	}

	template <int W> RISCV_INTERNAL
	void Memory<W>::binary_load_ph(const MachineOptions<W>& options, const Phdr* hdr)
	{
		const auto* src = m_binary.data() + hdr->p_offset;
		const size_t len = hdr->p_filesz;
		if (m_binary.size() <= hdr->p_offset ||
			hdr->p_offset + len < hdr->p_offset)
		{
			throw MachineException(INVALID_PROGRAM, "Bogus ELF program segment offset");
		}
		if (m_binary.size() < hdr->p_offset + len) {
			throw MachineException(INVALID_PROGRAM, "Not enough room for ELF program segment");
		}
		if (hdr->p_vaddr + len < hdr->p_vaddr) {
			throw MachineException(INVALID_PROGRAM, "Bogus ELF segment virtual base");
		}

		if (options.verbose_loader) {
		printf("* Loading program of size %zu from %p to virtual %p\n",
				len, src, (void*) (uintptr_t) hdr->p_vaddr);
		}
		// Serialize pages cannot be called with len == 0,
		// and there is nothing further to do.
		if (UNLIKELY(len == 0))
			return;

		// segment permissions
		const PageAttributes attr {
			 .read  = (hdr->p_flags & PF_R) != 0,
			 .write = (hdr->p_flags & PF_W) != 0,
			 .exec  = (hdr->p_flags & PF_X) != 0
		};
		if (options.verbose_loader) {
		printf("* Program segment readable: %d writable: %d  executable: %d\n",
				attr.read, attr.write, attr.exec);
		}

		if (attr.exec && m_exec.empty())
		{
			serialize_execute_segment(options, hdr);
			// Nothing more to do here, if execute-only
			if (!attr.read)
				return;
		}
		// We would normally never allow this
		if (attr.exec && attr.write) {
			if (!options.allow_write_exec_segment) {
				throw MachineException(INVALID_PROGRAM,
					"Insecure ELF has writable executable code");
			}
		}
		// In some cases we want to enforce execute-only
		if (attr.exec && (attr.read || attr.write)) {
			if (options.enforce_exec_only) {
				throw MachineException(INVALID_PROGRAM, "Execute segment must be execute-only");
			}
		}

		if (attr.read && !attr.write && m_ropages.end == 0) {
			// If the serialization fails, we will fallback to memcpy
			// with set_page_attr, like normal.
			if (serialize_pages(m_ropages, hdr->p_vaddr, src, len, attr))
				return;
		}

		// Load into virtual memory
		this->memcpy(hdr->p_vaddr, src, len);

		if (options.protect_segments) {
			this->set_page_attr(hdr->p_vaddr, len, attr);
		}
		else {
			// this might help execute simplistic barebones programs
			this->set_page_attr(hdr->p_vaddr, len, {
				 .read = true, .write = true, .exec = true
			});
		}
	}

	template <int W> RISCV_INTERNAL
	void Memory<W>::serialize_execute_segment(
		const MachineOptions<W>& options, const Phdr* hdr)
	{
		// The execute segment:
		address_t vaddr = hdr->p_vaddr;
		size_t exlen = hdr->p_filesz;
		const char* data = m_binary.data() + hdr->p_offset;
		// Look for a .text section inside this segment:
		const auto* texthdr = section_by_name(".text");
		if (texthdr != nullptr
			// Validate that the .text section is inside this
			// execute segment.
			&& texthdr->sh_addr >= vaddr && texthdr->sh_size <= exlen
			&& texthdr->sh_addr + texthdr->sh_size <= vaddr + exlen)
		{
			// Now we can use the .text section instead
			data = m_binary.data() + texthdr->sh_offset;
			vaddr = texthdr->sh_addr;
			exlen = texthdr->sh_size;
		}

		auto& exec_segment =
			this->create_execute_segment(options, data, vaddr, exlen);
		// Select the first execute segment
		machine().cpu.set_execute_segment(&exec_segment);
	}

	template <int W> RISCV_INTERNAL
	bool Memory<W>::serialize_pages(MemoryArea& area,
		address_t addr, const char* src, size_t size, PageAttributes attr)
	{
		static constexpr address_t PSIZEMASK = Page::size()-1;
		// It is not an optimization to store 1-2 pages
		if (size < 2*Page::size()) {
			area.pages = nullptr;
			area.begin = 0;
			area.end = 0;
			return false;
		}

		const address_t prebase = addr & ~PSIZEMASK;
		const address_t prelen  = addr - prebase;
		const address_t postbase = (addr + size) & ~PSIZEMASK;
		const address_t lastpage_len = (addr + size) - postbase;
		const address_t postlen = (lastpage_len > 0) ? Page::size() - lastpage_len : 0;
		// The total length should be a page-sized length
		const address_t total_len = prelen + size + postlen;
		assert((total_len & ~PSIZEMASK) == total_len);

		// Create the first and last page
		// TODO: Make this a PageData struct for alignment guarantees
		auto* pagedata = new uint8_t[Page::size() * 2] {};
		// Fill in the first page (at page 0)
		std::memset(&pagedata[0], 0,   prelen);
		std::memcpy(&pagedata[prelen], src,  Page::size() - prelen);
		// Fill in the last page (at page 1)
		std::memset(&pagedata[Page::size() + lastpage_len], 0,   postlen);
		std::memcpy(&pagedata[Page::size()], &src[postbase - addr], lastpage_len);
		area.data.reset(pagedata);

		const size_t npages = total_len / Page::size();
		area.pages.reset(new Page[npages]);
		// Create share-able range
		area.begin = addr / Page::size();
		area.end   = area.begin + npages;

		for (size_t i = 0; i < npages; i++) {
			area.pages[i].attr = attr;
			// None of the pages own their page memory
			area.pages[i].attr.non_owning = true;

			// We have custom page data for the first and last page
			if (i == 0 || i == npages-1) {
				const size_t offset = (i == 0) ? 0 : Page::size();
				area.pages[i].m_page.reset((PageData*) &pagedata[offset]);
			} else {
				const size_t offset = i * Page::size() - prelen;
				area.pages[i].m_page.reset((PageData*) &src[offset]);
			}
		}
		return true;
	}

	// ELF32 and ELF64 loader
	template <int W> RISCV_INTERNAL
	void Memory<W>::binary_loader(const MachineOptions<W>& options)
	{
		if (UNLIKELY(m_binary.size() < sizeof(Ehdr))) {
			throw MachineException(INVALID_PROGRAM, "ELF program too short");
		}
		const auto* elf = (Ehdr*) m_binary.data();
		if (UNLIKELY(!validate_header<Ehdr> (elf))) {
			throw MachineException(INVALID_PROGRAM, "Invalid ELF header! Mixup between 32- and 64-bit?", elf->e_ident[EI_CLASS]);
		}
		if (UNLIKELY(elf->e_type != ET_EXEC)) {
			throw MachineException(INVALID_PROGRAM, "ELF program is not an executable type. Trying to load a dynamic library?");
		}
		if (UNLIKELY(elf->e_machine != EM_RISCV)) {
			throw MachineException(INVALID_PROGRAM, "ELF program is not a RISC-V executable. Wrong architecture.");
		}

		// Enumerate & validate loadable segments
		const auto program_headers = elf->e_phnum;
		if (UNLIKELY(program_headers <= 0)) {
			throw MachineException(INVALID_PROGRAM, "ELF with no program-headers");
		}
		if (UNLIKELY(program_headers >= 16)) {
			throw MachineException(INVALID_PROGRAM, "ELF with too many program-headers");
		}
		if (UNLIKELY(elf->e_phoff > 0x4000)) {
			throw MachineException(INVALID_PROGRAM, "ELF program-headers have bogus offset");
		}
		if (UNLIKELY(elf->e_phoff + program_headers * sizeof(Phdr) > m_binary.size())) {
			throw MachineException(INVALID_PROGRAM, "ELF program-headers are outside the binary");
		}

		// Load program segments
		const auto* phdr = (Phdr*) (m_binary.data() + elf->e_phoff);
		this->m_start_address = elf->e_entry;
		this->m_heap_address = 0;

		for (const auto* hdr = phdr; hdr < phdr + program_headers; hdr++)
		{
			// Detect overlapping segments
			for (const auto* ph = phdr; ph < hdr; ph++) {
				if (hdr->p_type == PT_LOAD && ph->p_type == PT_LOAD)
				if (ph->p_vaddr < hdr->p_vaddr + hdr->p_filesz &&
					ph->p_vaddr + ph->p_filesz > hdr->p_vaddr) {
					// Normally we would not care, but no normal ELF
					// has overlapping segments, so treat as bogus.
					throw MachineException(INVALID_PROGRAM, "Overlapping ELF segments");
				}
			}

			switch (hdr->p_type)
			{
				case PT_LOAD:
					// loadable program segments
					if (options.load_program) {
						binary_load_ph(options, hdr);
					}
					break;
				case PT_GNU_STACK:
					// This seems to be a mark for executable stack. Big NO!
					break;
				case PT_GNU_RELRO:
					//throw std::runtime_error(
					//	"Dynamically linked ELF binaries are not supported");
					break;
			}

			address_t endm = hdr->p_vaddr + hdr->p_memsz;
			endm += Page::size()-1; endm &= ~address_t(Page::size()-1);
			if (this->m_heap_address < endm)
				this->m_heap_address = endm;
		}

		// The base mmap address starts at heap start + BRK_MAX
		// TODO: We should check if the heap starts too close to the end
		// of the address space now, and move it around if necessary.
		this->m_mmap_address = m_heap_address + BRK_MAX;

		// Default stack
		this->m_stack_address = mmap_allocate(options.stack_size) + options.stack_size;

		// Insert host code page, with exit function, enabling VM calls.
		auto host_page = this->mmap_allocate(Page::size());
		this->install_shared_page(page_number(host_page), Page::host_page());
		this->m_exit_address = host_page;

		if constexpr (W <= 8) {
			if (options.dynamic_linking) {
				this->dynamic_linking();
			}
		}

		if (options.verbose_loader) {
			printf("* Entry is at %p\n",
				(void*)uintptr_t(this->start_address()));
		}
	}

	template <int W> RISCV_INTERNAL
	void Memory<W>::machine_loader(
		const Machine<W>& master, const MachineOptions<W>& options)
	{
		// Some machines don't need custom PF handlers
		this->m_page_fault_handler = master.memory.m_page_fault_handler;

		if (options.minimal_fork == false)
		{
			// Hardly any pages are dont_fork, so we estimate that
			// all master pages will be loaned.
			m_pages.reserve(master.memory.pages().size());
			for (const auto& it : master.memory.pages())
			{
				const auto& page = it.second;
				// Skip pages marked as dont_fork
				if (page.attr.dont_fork) continue;
				// Make every page non-owning
				auto attr = page.attr;
				if (attr.write) {
					attr.write = false;
					attr.is_cow = true;
				}
				attr.non_owning = true;
				m_pages.emplace(std::piecewise_construct,
					std::forward_as_tuple(it.first),
					std::forward_as_tuple(attr, page.m_page.get())
				);
			}
		}
		this->m_start_address = master.memory.m_start_address;
		this->m_stack_address = master.memory.m_stack_address;
		this->m_exit_address = master.memory.m_exit_address;
		this->m_heap_address = master.memory.m_heap_address;
		this->m_mmap_address = master.memory.m_mmap_address;

		// TODO: Set callback that can loan execute segments from master

		this->m_ropages.begin = master.memory.m_ropages.begin;
		this->m_ropages.end   = master.memory.m_ropages.end;
		this->m_ropages.pages.reset(master.memory.m_ropages.pages.get());

		// invalidate all cached pages, because references are invalidated
		this->invalidate_reset_cache();
	}

	template <int W>
	std::string Memory<W>::get_page_info(address_t addr) const
	{
		char buffer[1024];
		int len;
		if constexpr (W == 4) {
			len = snprintf(buffer, sizeof(buffer),
				"[0x%08" PRIX32 "] %s", addr, get_page(addr).to_string().c_str());
		} else if constexpr (W == 8) {
			len = snprintf(buffer, sizeof(buffer),
				"[0x%016" PRIX64 "] %s", addr, get_page(addr).to_string().c_str());
		} else if constexpr (W == 16) {
			len = snprintf(buffer, sizeof(buffer),
				"[0x%016" PRIX64 "] %s", (uint64_t)addr, get_page(addr).to_string().c_str());
		}
		return std::string(buffer, len);
	}

	template <int W>
	typename Memory<W>::Callsite Memory<W>::lookup(address_t address) const
	{
		const auto* sym_hdr = section_by_name(".symtab");
		if (sym_hdr == nullptr) return {};
		const auto* str_hdr = section_by_name(".strtab");
		if (str_hdr == nullptr) return {};
		// backtrace can sometimes find null addresses
		if (address == 0x0) return {};
		// ELF with no symbols
		if (UNLIKELY(sym_hdr->sh_size == 0)) return {};

		const auto* symtab = elf_sym_index(sym_hdr, 0);
		const size_t symtab_ents = sym_hdr->sh_size / sizeof(typename Elf<W>::Sym);
		const char* strtab = elf_offset<char>(str_hdr->sh_offset);

		const auto result =
			[] (const char* strtab, address_t addr, const auto* sym)
		{
			const char* symname = &strtab[sym->st_name];
#ifdef DEMANGLE_ENABLED
			char* dma = __cxa_demangle(symname, nullptr, nullptr, nullptr);
#else
			char* dma = nullptr;
#endif
			return Callsite {
				.name = (dma) ? dma : symname,
				.address = sym->st_value,
				.offset = (uint32_t) (addr - sym->st_value),
				.size   = (size_t)sym->st_size
			};
		};

		const typename Elf<W>::Sym* best = nullptr;
		for (size_t i = 0; i < symtab_ents; i++)
		{
			if (ELF32_ST_TYPE(symtab[i].st_info) != STT_FUNC) continue;
			/*printf("Testing %#X vs  %#X to %#X = %s\n",
					address, symtab[i].st_value,
					symtab[i].st_value + symtab[i].st_size, symname);*/

			if (address >= symtab[i].st_value &&
				address < symtab[i].st_value + symtab[i].st_size)
			{
				// exact match
				return result(strtab, address, &symtab[i]);
			}
			else if (address > symtab[i].st_value)
			{
				// best guess (symbol + 0xOff)
				best = &symtab[i];
			}
		}
		if (best)
			return result(strtab, address, best);
		return {};
	}
	template <int W>
	void Memory<W>::print_backtrace(
		std::function<void(std::string_view)> print_function)
	{
		auto print_trace =
			[this, print_function] (const int N, const address_type<W> addr) {
				// get information about the callsite
				const auto site = this->lookup(addr);
				// write information directly to stdout
				char buffer[8192];
				int len;
				if constexpr (W == 4) {
					len = snprintf(buffer, sizeof(buffer),
						"[%d] 0x%08" PRIx32 " + 0x%.3" PRIx32 ": %s",
						N, site.address, site.offset, site.name.c_str());
				} else if constexpr (W == 8) {
					len = snprintf(buffer, sizeof(buffer),
						"[%d] 0x%016" PRIX64 " + 0x%.3" PRIx32 ": %s",
						N, site.address, site.offset, site.name.c_str());
				} else if constexpr (W == 16) {
					len = snprintf(buffer, sizeof(buffer),
						"[%d] 0x%016" PRIx64 " + 0x%.3" PRIx32 ": %s",
						N, (uint64_t)site.address, site.offset, site.name.c_str());
				}
				if (len > 0)
					print_function({buffer, (size_t)len});
				else
					print_function("Scuffed frame. Should not happen!");
			};
		print_trace(0, this->machine().cpu.pc());
		print_trace(1, this->machine().cpu.reg(REG_RA));
	}

	template <int W>
	void Memory<W>::protection_fault(address_t addr)
	{
		CPU<W>::trigger_exception(PROTECTION_FAULT, addr);
	}

	template struct Memory<4>;
	template struct Memory<8>;
	INSTANTIATE_128_IF_ENABLED(Memory);
}