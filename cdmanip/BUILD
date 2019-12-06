package(default_visibility = ["//cdmanip:internal"])

package_group(
    name = "internal",
    packages = ["//cdmanip/..."],
)

cc_binary(
    name = "cdmanip",
    srcs = ["cdmanip.cc"],
    deps = [
        #":cuesheet",
        #":hash",
        "//cdmanip/subcommands:all_subcommands",
        "//cdmanip/subcommands",
        "@rhutil//rhutil:status",
        "@rhutil//rhutil:module_init",
        "@abseil//absl/strings",
        "@abseil//absl/types:span",
        "@abseil//absl/flags:parse",
    ],
)

#cc_library(
#    name = "cuesheet",
#    srcs = ["cuesheet.cc"],
#    hdrs = ["cuesheet.h"],
#    deps = [
#        "//cdmanip/cdmap:cdmap_cc_proto",
#        "@absl//absl/strings",
#        "@absl//absl/strings:str_format",
#        "@absl//absl/types:optional",
#        "@absl//absl/algorithm:container",
#    ],
#)

cc_library(
    name = "common_flags",
    srcs = ["common_flags.cc"],
    hdrs = ["common_flags.h"],
    deps = ["@abseil//absl/flags:flag"],
)
