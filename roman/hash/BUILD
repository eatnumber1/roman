package(default_visibility = ["//roman:internal"])

cc_library(
    name = "md5",
    srcs = ["md5.cc"],
    hdrs = ["md5.h"],
    deps = [
        "@absl//absl/strings",
        "@absl//absl/types:optional",
        "@boringssl//:crypto",
    ],
)
