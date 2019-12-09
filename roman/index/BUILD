package(default_visibility = ["//roman:internal"])

cc_library(
    name = "game_indexer",
    srcs = ["game_indexer.cc"],
    hdrs = ["game_indexer.h"],
    deps = [
        ":game_index_cc_proto",
        "//roman:hash",
        "@abseil//absl/container:flat_hash_map",
        "@rhutil//rhutil:status",
        "@dat2pb//dat2pb:romdat_cc_proto",
    ],
)

cc_proto_library(
    name = "game_index_cc_proto",
    deps = [":game_index_proto"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "game_index_proto",
    srcs = ["game_index.proto"],
    deps = [
        "@dat2pb//dat2pb:romdat_proto",
    ],
    visibility = ["//visibility:public"],
)
