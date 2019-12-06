package(default_visibility = ["//cdmanip:internal"])

cc_library(
    name = "fs",
    srcs = ["fs.cc"],
    hdrs = ["fs.h"],
    deps = [
        "@abseil//absl/time",
        "@abseil//absl/container:flat_hash_map",
        #"@hash_library//:crc32",
        "@rhutil//rhutil:status",
    ],
)
