#[cxx::bridge]
mod ffi{
    extern "C++"{
        include!("cc_main.h");
    }
}
enum Handle<'a,'rt>{
    InSubsystem {module: &'a wasm3::Module<'rt>, ptr: u32}
}