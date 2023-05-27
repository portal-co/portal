use std::{path::Path, process::Command};

pub fn mk(x: String) {
    let r = std::env::var("R").unwrap();
    Command::new("/usr/bin/env")
        .arg("make")
        .arg("-C")
        .arg(r)
        .arg(x.clone())
        .spawn()
        .expect(&format!("{x} failed to build"))
        .wait()
        .expect(&format!("{x} failed to build"));
}

pub fn libs(x: &[&str], bridge: impl IntoIterator<Item = impl AsRef<Path>>) -> cc::Build {
    let mut b = cxx_build::bridges(bridge);
    let r = std::env::var("R").unwrap();
    for y in x {
        match y.to_owned() {
            "wasm3" => {
                mk(format!("{r}/3rdparty-build-wasm/wasm3/libm3.a"));
                b.include(format!("{r}/3rdparty.o/wasm3/source"))
                    .include(format!("{r}/tools/wasm3/include"))
                    .object(format!("{r}/3rdparty-build-wasm/wasm3/libm3.a"));
            }
            "binaryen" => {
                mk(format!(
                    "{r}/3rdparty-build-wasm/binaryen/lib/libbinaryen.a"
                ));
                b.include(format!("{r}/3rdparty.o/binaryen/src"))
                    .object(format!(
                        "{r}/3rdparty-build-wasm/binaryen/lib/libbinaryen.a"
                    ));
            }
            "risc" => {
                mk(format!("{r}/3rdparty-build-wasm/libriscv/libriscv.a"));
                b.object(format!("{r}/3rdparty-build-wasm/libriscv/libriscv.a"))
                    .include(format!("{r}/3rdparty.o/libriscv/lib/libriscv"));
            }
            _ => {}
        }
    }
    return b;
}
