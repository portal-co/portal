load(":lgen.bzl","lgen")
cc_binary(
    name = "lgen",
    srcs = ["gen.cc","lc.h"],
    visibility = ["//visibility:public"]
)
lgen(
    name = "test1",
    lam = "test1.lc"
)