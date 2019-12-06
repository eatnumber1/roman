package(default_visibility = ["//roman:internal"])

cc_library(
    name = "cdm_writer",
    hdrs = ["cdm_writer.h"],
    srcs = ["cdm_writer.cc"],
    deps = [
        ":flac_filter",
        "//roman/util:errors",
        "//roman/util:strings",
        "//roman/util:glib",
        #"//roman:cuesheet",
        "//roman/cdmap:cdmap_cc_proto",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        #"@absl//absl/algorithm:container",
        "@absl//absl/types:span",
        #"@absl//absl/flags:flag",
        #"@absl//absl/strings:str_format",
        #"@absl//absl/container:fixed_array",
        #"@com_google_protobuf//:protobuf",
        "@libmirage//:libmirage_nolib",
        "@glib//:glib",
        "@flac//:libflac",
    ],
)

cc_library(
    name = "flac_filter",
    hdrs = ["flac_filter.h"],
    srcs = ["flac_filter.cc"],
    deps = [
        "//roman/util:errors",
        "//roman/util:strings",
        "//roman/util:glib",
        #"//roman:cuesheet",
        "//roman/cdmap:cdmap_cc_proto",
        "@absl//absl/strings",
        "@absl//absl/strings:str_format",
        #"@absl//absl/algorithm:container",
        "@absl//absl/types:span",
        #"@absl//absl/flags:flag",
        #"@absl//absl/strings:str_format",
        "@absl//absl/container:fixed_array",
        "@absl//absl/types:optional",
        #"@com_google_protobuf//:protobuf",
        "@libmirage//:libmirage_nolib",
        "@glib//:glib",
        "@flac//:libflac",
    ],
)

cc_binary(
    name = "image-cdm.so",
    srcs = ["cdm_plugin.cc"],
    deps = [
        ":cdm_writer",
        "@glib//:glib",
        "@libmirage//:libmirage_nolib",
    ],
    linkstatic = True,
    linkshared = True,
)
