package(default_visibility = ["//cdmanip:internal"])

cc_library(
    name = "subcommands",
    hdrs = ["subcommands.h"],
    srcs = [
        "dump_cdmap.cc",
        "parse_cuesheet.cc",
        "expand_tracks.cc",
    ],
    deps = [
        "//cdmanip/cdmap:cdmap_cc_proto",
        "//cdmanip:cuesheet",
        "//cdmanip/cdmap",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        "@absl//absl/container:fixed_array",
        "@com_google_protobuf//:protobuf",
    ],
)
