load(":lgen.bzl","lgen")
cc_binary(
    name = "lgen",
    srcs = ["gen.cc","lc.h"],
    visibility = ["//visibility:public"],
    deps = [":csb"]
)
lgen(
    name = "test1",
    lams = ["test1.lc"]
)
cc_library(
    name = "csb",
    hdrs = ["csb.h"]
)
filegroup(
    name = "base",
    srcs = ["base.lc"],
    visibility = ["//visibility:public"],
)