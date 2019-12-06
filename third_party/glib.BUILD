cc_library(
    name = "glib",
    srcs = ["lib/libglib-2.0.dylib", "lib/libgobject-2.0.dylib"],
    hdrs = glob(["include/glib-2.0/**/*.h",
                 "lib/glib-2.0/include/**/*.h"]),
    includes = ["include/glib-2.0", "lib/glib-2.0/include"],
    visibility = ["//visibility:public"],
)

#cc_library(
#    name = "glib",
#    srcs = ["lib/libglib-2.0.dylib", "lib/libgobject-2.0.dylib"],
#    hdrs = glob(["include/glib-2.0/**/*.h"]),
#    include_prefix = "glib",
#    strip_include_prefix = "include/glib-2.0",
#    includes = ["include/glib-2.0", "lib/glib-2.0/include"],
#    visibility = ["//visibility:public"],
#    deps = [":glibconfig"],
#)
#
#cc_library(
#    name = "glibconfig",
#    hdrs = ["lib/glib-2.0/include/glibconfig.h"],
#    strip_include_prefix = "lib/glib-2.0/include",
#    includes = ["lib/glib-2.0/include"],
#    include_prefix = "glib",
#)
