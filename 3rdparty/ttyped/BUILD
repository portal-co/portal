load("@rules_haskell//haskell:cabal.bzl", "haskell_cabal_binary")
load("@stackage//:packages.bzl", "packages")
haskell_cabal_binary(
    name = "ttyped",
    srcs = glob(["**"]),
    deps = packages["ttyped"].deps,
    visibility = ["//visibility:public"],
    tools = ["@stackage-exe//alex"]
)
