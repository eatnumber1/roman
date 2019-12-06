load("@rules_foreign_cc//tools/build_defs:cmake.bzl", "cmake_external")

filegroup(
    name = "all",
    srcs = glob(["**"]),
)

cmake_external(
    name = "libzip",
    cache_entries = {
        "ENABLE_COMMONCRYPTO": "OFF",
        "ENABLE_GNUTLS": "OFF",
        "ENABLE_MBEDTLS": "OFF",
        "ENABLE_OPENSSL": "OFF",
        "ENABLE_WINDOWS_CRYPTO": "OFF",
        "ENABLE_BZIP2": "OFF",

        "BUILD_TOOLS": "OFF",
        "BUILD_EXAMPLES": "OFF",
        "BUILD_DOC": "OFF",
        "BUILD_SHARED_LIBS": "OFF",
        "BUILD_REGRESS": "OFF",
    },
    lib_source = ":all",
    visibility = ["//visibility:public"],
    deps = ["@zlib//:zlib"],
    make_commands = [
        "make -j4 zip",
        "make -j4 install",
    ],
)
