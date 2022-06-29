load("@x_3rdparty//ttyped:tty.bzl","ttgen")
load("//lgen:lgen.bzl","lgen")
ttgen(name = "ttg", srcs = ["@x_3rdparty//ttyped/lib"])
lgen(name = "knl", lams = [":ttg","base.lc"])