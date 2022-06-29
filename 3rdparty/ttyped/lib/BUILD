load("//ttyped:tty.bzl","ttgen")
ttgen(name = "test-lib", srcs = glob(["*.tt"]))
filegroup(name = "lib", srcs = glob(["*.tt"]), visibility = ["//visibility:public"])