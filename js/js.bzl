DEFAULT_EMSCRIPTEN_LINKOPTS = [
    # LTO broken when using hermetic emsdk
    # "-flto",                            # Specify lto (has to be set on for compiler as well)
    "--bind",                           # Compiles the source code using the Embind bindings to connect C/C++ and JavaScript
    # Closure compiler broken when using hermetic emsdk
    # "--closure 1",                      # Run the closure compiler
    "-s MALLOC=emmalloc",               # Switch to using the much smaller implementation
    "-s ASSERTIONS=0",                  # Turn off assertions
    "-s EXPORT_ES6=1",                  # Export as es6 module, used for rollup
    "-s MODULARIZE=1",                  # Allows us to manually invoke the initializatio of wasm
    "-s EXPORT_NAME=createModule",      # Not used, but good to specify
    "-s USE_ES6_IMPORT_META=0",         # Disable loading from import meta since we use rollup
    "-s SINGLE_FILE=1",                 # Pack all webassembly into base64
    "-s NODEJS_CATCH_EXIT=0",           # We don't have a 'main' so disable exit() catching
    "-fexceptions"
]
ASMJS_LINKOPTS = [
    "-s WASM=0",                    # Specify asm.js output
]

WASM_LINKOPTS = [
    "-s WASM=1",                    # Specify wasm output
]
ems_linkopts = DEFAULT_EMSCRIPTEN_LINKOPTS + ASMJS_LINKOPTS
ems_copts = ["-fexceptions"]