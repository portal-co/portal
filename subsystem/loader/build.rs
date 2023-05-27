fn main() {
    let r = std::env::var("R").unwrap();
    libs_all::libs(&["wasm3"], vec!["src/lib.rs"]) // returns a cc::Build
        .file("src/cc_main.cc")
        .flag_if_supported("-std=c++2a")
        .compile("ss-loader");
}