package(default_visibility = ["//roman:internal"])

package_group(
    name = "internal",
    packages = ["//roman/..."],
)

cc_binary(
    name = "roman",
    srcs = ["roman.cc"],
    deps = [
        #":cuesheet",
        #":hash",
        "//roman/subcommands:all_subcommands",
        "//roman/subcommands",
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
#        "//roman/cdmap:cdmap_cc_proto",
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

cc_library(
    name = "hash",
    srcs = ["hash.cc"],
    hdrs = ["hash.h"],
)

cc_library(
    name = "print_proto",
    srcs = ["print_proto.cc"],
    hdrs = ["print_proto.h"],
    deps = [
        "@rhutil//rhutil:status",
        "@abseil//absl/flags:flag",
        "@com_google_protobuf//:protobuf",
    ],
)
