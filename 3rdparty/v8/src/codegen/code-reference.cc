// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/codegen/code-reference.h"

#include "src/codegen/code-desc.h"
#include "src/common/globals.h"
#include "src/handles/handles-inl.h"
#include "src/objects/objects-inl.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/wasm-code-manager.h"
#endif  // V8_ENABLE_WEBASSEMBLY

namespace v8 {
namespace internal {

namespace {
struct JSOps {
  Handle<Code> code;

  Address constant_pool() const { return code->constant_pool(); }
  Address instruction_start() const { return code->InstructionStart(); }
  Address instruction_end() const { return code->InstructionEnd(); }
  int instruction_size() const { return code->InstructionSize(); }
  const byte* relocation_start() const { return code->relocation_start(); }
  const byte* relocation_end() const { return code->relocation_end(); }
  int relocation_size() const { return code->relocation_size(); }
  Address code_comments() const { return code->code_comments(); }
  int code_comments_size() const { return code->code_comments_size(); }
};

#if V8_ENABLE_WEBASSEMBLY
struct WasmOps {
  const wasm::WasmCode* code;

  Address constant_pool() const { return code->constant_pool(); }
  Address instruction_start() const {
    return reinterpret_cast<Address>(code->instructions().begin());
  }
  Address instruction_end() const {
    return reinterpret_cast<Address>(code->instructions().begin() +
                                     code->instructions().size());
  }
  int instruction_size() const { return code->instructions().length(); }
  const byte* relocation_start() const { return code->reloc_info().begin(); }
  const byte* relocation_end() const {
    return code->reloc_info().begin() + code->reloc_info().length();
  }
  int relocation_size() const { return code->reloc_info().length(); }
  Address code_comments() const { return code->code_comments(); }
  int code_comments_size() const { return code->code_comments_size(); }
};
#endif  // V8_ENABLE_WEBASSEMBLY

struct CodeDescOps {
  const CodeDesc* code_desc;

  Address constant_pool() const {
    return instruction_start() + code_desc->constant_pool_offset;
  }
  Address instruction_start() const {
    return reinterpret_cast<Address>(code_desc->buffer);
  }
  Address instruction_end() const {
    return instruction_start() + code_desc->instr_size;
  }
  int instruction_size() const { return code_desc->instr_size; }
  const byte* relocation_start() const {
    return code_desc->buffer + code_desc->reloc_offset;
  }
  const byte* relocation_end() const {
    return code_desc->buffer + code_desc->buffer_size;
  }
  int relocation_size() const { return code_desc->reloc_size; }
  Address code_comments() const {
    return instruction_start() + code_desc->code_comments_offset;
  }
  int code_comments_size() const { return code_desc->code_comments_size; }
};
}  // namespace

#if V8_ENABLE_WEBASSEMBLY
#define DISPATCH(ret, method)                    \
  ret CodeReference::method() const {            \
    DCHECK(!is_null());                          \
    switch (kind_) {                             \
      case Kind::JS:                             \
        return JSOps{js_code_}.method();         \
      case Kind::WASM:                           \
        return WasmOps{wasm_code_}.method();     \
      case Kind::CODE_DESC:                      \
        return CodeDescOps{code_desc_}.method(); \
      default:                                   \
        UNREACHABLE();                           \
    }                                            \
  }
#else
#define DISPATCH(ret, method)                              \
  ret CodeReference::method() const {                      \
    DCHECK(!is_null());                                    \
    DCHECK(kind_ == Kind::JS || kind_ == Kind::CODE_DESC); \
    if (kind_ == Kind::JS) {                               \
      return JSOps{js_code_}.method();                     \
    } else {                                               \
      return CodeDescOps{code_desc_}.method();             \
    }                                                      \
  }
#endif  // V8_ENABLE_WEBASSEMBLY

DISPATCH(Address, constant_pool)
DISPATCH(Address, instruction_start)
DISPATCH(Address, instruction_end)
DISPATCH(int, instruction_size)
DISPATCH(const byte*, relocation_start)
DISPATCH(const byte*, relocation_end)
DISPATCH(int, relocation_size)
DISPATCH(Address, code_comments)
DISPATCH(int, code_comments_size)

#undef DISPATCH

}  // namespace internal
}  // namespace v8
