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
    packages = [],
    # LTS snapshot published for ghc-8.10.7 (default version used by rules_haskell)
    snapshot = "lts-18.18",
    # This uses an unpinned version of stack_snapshot, meaning that stack is invoked on every build.
    # To switch to pinned stackage dependencies, run \`bazel run @stackage-unpinned//:pin\` and
    # uncomment the following line.
    vendored_packages = {"ttypes": "//3rdparty/ttypes:ttypes"}
)