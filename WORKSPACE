local_repository(
    name = "x_3rdparty",
    path = "./3rdparty"
)

load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive"
)

http_archive(
    name = "rules_haskell",
    strip_prefix = "rules_haskell-0.14",
    urls = ["https://github.com/tweag/rules_haskell/archive/v0.14.tar.gz"],
    sha256 = "851e16edc7c33b977649d66f2f587071dde178a6e5bcfeca5fe9ebbe81924334",
)
http_archive(
    name = "rules_sh",
    sha256 = "83a065ba6469135a35786eb741e17d50f360ca92ab2897857475ab17c0d29931",
    strip_prefix = "rules_sh-0.2.0",
    urls = ["https://github.com/tweag/rules_sh/archive/v0.2.0.tar.gz"],
)
load("@rules_sh//sh:repositories.bzl", "rules_sh_dependencies")
rules_sh_dependencies()
load(
    "@rules_haskell//haskell:repositories.bzl",
    "rules_haskell_dependencies",
)
# Setup all Bazel dependencies required by rules_haskell.
rules_haskell_dependencies()
load(
    "@rules_haskell//haskell:toolchain.bzl",
    "rules_haskell_toolchains",
)
rules_haskell_toolchains()
load(
    "@rules_haskell//haskell:cabal.bzl",
    "stack_snapshot"
)

stack_snapshot(
    name = "stackage",
    packages = ["alex","she-0.6"],
    # LTS snapshot published for ghc-8.10.7 (default version used by rules_haskell)
    snapshot = "lts-18.18",
    # This uses an unpinned version of stack_snapshot, meaning that stack is invoked on every build.
    # To switch to pinned stackage dependencies, run \`bazel run @stackage-unpinned//:pin\` and
    # uncomment the following line.
    vendored_packages = {"ttyped": "@x_3rdparty//ttyped:ttyped"}
)
# emsdk 2.0.31
http_archive(
    name = "emsdk",
    strip_prefix = "emsdk-3891e7b04bf8cbb3bc62758e9c575ae096a9a518/bazel",
    url = "https://github.com/emscripten-core/emsdk/archive/3891e7b04bf8cbb3bc62758e9c575ae096a9a518.tar.gz",
    sha256 = "d55e3c73fc4f8d1fecb7aabe548de86bdb55080fe6b12ce593d63b8bade54567",
)

load("@emsdk//:deps.bzl", emsdk_deps = "deps")
emsdk_deps()

load("@emsdk//:emscripten_deps.bzl", emsdk_emscripten_deps = "emscripten_deps")
emsdk_emscripten_deps()