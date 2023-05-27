mkdir -p cc/wasi
cd cc/wasi
export WASI_VERSION=20
export WASI_VERSION_FULL=${WASI_VERSION}.0
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}/wasi-sdk-${WASI_VERSION_FULL}-linux.tar.gz
tar xvf wasi-sdk-${WASI_VERSION_FULL}-linux.tar.gz
ln -s $(pwd)/wasi-sdk-20.0/share/wasi-sysroot/include  $(pwd)/wasi-sdk-20.0/share/wasi-sysroot/include/wasm32-wasi-threads