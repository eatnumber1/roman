package(default_visibility = ["//roman:internal"])

cc_library(
    name = "fetch",
    alwayslink = 1,
    srcs = ["fetch.cc"],
    deps = [
        ":subcommands",
        "//roman:common_flags",
        "//roman:print_proto",
        "@abseil//absl/flags:flag",
        "@abseil//absl/strings",
        "@abseil//absl/strings:str_format",
        "@abseil//absl/types:span",
        "@dat2pb//dat2pb:parser",
        "@dat2pb//dat2pb:romdat_cc_proto",
        "@libzip//:libzip",
        "@rhutil//rhutil/curl",
        "@rhutil//rhutil:module_init",
        "@rhutil//rhutil:status",
    ],
)

cc_library(
    name = "verify",
    alwayslink = 1,
    srcs = ["verify.cc"],
    deps = [
        "//roman:common_flags",
        "//roman:hash",
        "//roman/index:game_indexer",
        ":subcommands",
        "@abseil//absl/container:flat_hash_map",
        "@abseil//absl/container:flat_hash_set",
        "@abseil//absl/flags:flag",
        "@abseil//absl/strings",
        "@abseil//absl/strings:str_format",
        "@abseil//absl/types:span",
        "@dat2pb//dat2pb:parser",
        "@dat2pb//dat2pb:romdat_cc_proto",
        "@nlohmann_json//:json",
        "@rcrc//rcrc:rclone",
        "@rhutil//rhutil:file",
        "@rhutil//rhutil:module_init",
        "@rhutil//rhutil:status",
    ],
)

cc_library(
    name = "index",
    alwayslink = 1,
    srcs = ["index.cc"],
    deps = [
        "//roman:common_flags",
        "//roman:hash",
        "//roman:print_proto",
        "//roman/index:game_indexer",
        ":subcommands",
        "@abseil//absl/container:flat_hash_map",
        "@abseil//absl/container:flat_hash_set",
        "@abseil//absl/flags:flag",
        "@abseil//absl/strings",
        "@abseil//absl/strings:str_format",
        "@abseil//absl/types:span",
        "@dat2pb//dat2pb:parser",
        "@dat2pb//dat2pb:romdat_cc_proto",
        "@nlohmann_json//:json",
        "@rcrc//rcrc:rclone",
        "@rhutil//rhutil:file",
        "@rhutil//rhutil:module_init",
        "@rhutil//rhutil:status",
    ],
)

cc_library(
    name = "printdatpb",
    alwayslink = 1,
    srcs = ["printdatpb.cc"],
    deps = [
        ":subcommands",
        "@rhutil//rhutil:module_init",
        "@rhutil//rhutil:file",
        "@rhutil//rhutil:status",
        "@abseil//absl/strings",
        "@abseil//absl/strings:str_format",
        "@abseil//absl/types:span",
        "@com_google_protobuf//:protobuf",
        "@dat2pb//dat2pb:parser",
        "@dat2pb//dat2pb:romdat_cc_proto",
    ],
)

cc_library(
    name = "subcommands",
    hdrs = ["subcommands.h"],
    srcs = ["subcommands.cc"],
    deps = [
        "@rhutil//rhutil:status",
        "@abseil//absl/types:span",
        "@abseil//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "all_subcommands",
    deps = [
        ":verify",
        ":fetch",
        ":printdatpb",
        ":index",
    ],
)
