
cc/wasi/wasi-sdk-20.0:
	sh ./wasi-sdk-get.sh

cc/r5/riscv.tgz:
	mkdir -p cc/r5
	curl http://musl.cc/riscv64-linux-musl-cross.tgz > cc/r5/riscv.tgz

cc/r5/sdk: cc/r5/riscv.tgz
	mkdir -p cc/r5/sdk
	tar -C $(R)/cc/r5/sdk -xvf $(R)/cc/r5/riscv.tgz

3rdparty.o/binaryen:
	git clone https://github.com/WebAssembly/binaryen.git 3rdparty.o/binaryen

3rdparty-build-wasm/binaryen/lib/libbinaryen.a: 3rdparty.o/binaryen cc/wasi/wasi-sdk-20.0
	mkdir -p 3rdparty-build-wasm/binaryen
	sh $(R)/tools/kcmake -DBUILD_TESTS=OFF -DENABLE_WERROR=OFF -DBUILD_TOOLS=OFF -DBUILD_LLVM_DWARF=OFF -DBUILD_STATIC_LIB=ON -S$(shell pwd)/3rdparty.o/binaryen -B$(shell pwd)/3rdparty-build-wasm/binaryen
	$(MAKE) -C 3rdparty-build-wasm/binaryen binaryen

3rdparty.o/wasm3:
	git clone https://github.com/wasm3/wasm3.git 3rdparty.o/wasm3
	cp $(R)/tools/wasm3/patch/* 3rdparty.o/wasm3/source

3rdparty-build-wasm/wasm3/libm3.a: 3rdparty.o/wasm3 cc/wasi/wasi-sdk-20.0
	mkdir -p 3rdparty-build-wasm/wasm3
	sh $(R)/tools/kcmake -S3rdparty.o/wasm3 -DBUILD_WASI=simple -B$(R)/3rdparty-build-wasm/wasm3
	$(MAKE) -C 3rdparty-build-wasm/wasm3 m3

3rdparty.o/libriscv:
	git clone https://github.com/fwsGonzo/libriscv.git 3rdparty.o/libriscv
	cat <(echo "cmake_minimum_required(VERSION 3.16)") 3rdparty.o/libriscv/CMakeLists.txt | sponge 3rdparty.o/libriscv/CMakeLists.txt
	cp ./tools/libriscv/patch/CMakeLists.txt 3rdparty.o/libriscv/lib
	cp ./tools/libriscv/patch/memory.cpp 3rdparty.o/libriscv/lib/libriscv


3rdparty-build-wasm/libriscv/libriscv.a: 3rdparty.o/libriscv
	mkdir -p 3rdparty-build-wasm/libriscv
	sh $(R)/tools/kcmake -S3rdparty.o/libriscv -B$(R)/3rdparty-build-wasm/libriscv
	$(MAKE) -C 3rdparty-build-wasm/libriscv riscv

WASM3_CFLAGS=-I3rdparty.o/wasm3/source -I$(R)/tools/wasm3/include

WASM3_LIBS=3rdparty.o-build-wasm/wasm3/lib/libm3.a

BY_CFLAGS=-I$(shell pwd)/3rdparty.o/binaryen/src

BY_LIBS=3rdparty-build-wasm/binaryen/lib/libbinaryen.a

RISC_LIBS=3rdparty-build-wasm/libriscv/libriscv.a

RISC_CFLAGS=-I$(shell pwd)/3rdparty.o/libriscv/lib/libriscv