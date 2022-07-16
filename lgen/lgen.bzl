load("//js:js.bzl","ems_linkopts","ems_copts")
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
def lgen(name,lams,visibility=["//visibility:public"]):
    native.genrule(name = name + "/gen", srcs = lams + ["//lgen:base"],tools = ["//lgen:lgen"], outs = ["lg.cc"], cmd = "cat $(SRCS) | tr -d '\n' | ./$(location //lgen:lgen) > $(OUTS)")
    native.cc_binary(name = name, srcs = [":" + name + "/gen"], visibility = visibility, deps = ["@x_3rdparty//:infint"])
    native.cc_binary(copts = ems_copts,linkopts = ems_linkopts,name = name + "/asmjs_bin", srcs = [":" + name + "/gen"], visibility = visibility, deps = ["@x_3rdparty//:infint"])
    wasm_cc_binary(name = name + "/asmjs", visibility = visibility, cc_target = ":" + name + "/asmjs_bin")