package(default_visibility = ["//cdmanip:internal"])

package_group(
    name = "internal",
    packages = ["//cdmanip/..."],
)

cc_binary(
    name = "cdmanip",
    srcs = ["cdmanip.cc"],
    deps = [
        ":cuesheet",
        ":hash",
        "//cdmanip/subcommands",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "hash",
    srcs = ["hash.cc"],
    hdrs = ["hash.h"],
    deps = [
        "@absl//absl/strings",
        "@boringssl//:crypto",
    ],
)

cc_library(
    name = "cuesheet",
    srcs = ["cuesheet.cc"],
    hdrs = ["cuesheet.h"],
    deps = [
        "//cdmanip/cdmap:cdmap_cc_proto",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:optional",
        "@absl//absl/algorithm:container",
    ],
)
