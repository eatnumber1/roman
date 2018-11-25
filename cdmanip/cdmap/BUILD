package(default_visibility = ["//cdmanip:internal"])

cc_library(
    name = "cdmap",
    srcs = ["cdmap.cc"],
    hdrs = ["cdmap.h"],
    deps = [
        ":cdmap_cc_proto",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
    ],
)

cc_proto_library(
    name = "cdmap_cc_proto",
    deps = [":cdmap_proto"],
)

proto_library(
    name = "cdmap_proto",
    srcs = ["cdmap.proto"],
)
