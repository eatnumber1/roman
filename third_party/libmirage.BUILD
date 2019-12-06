cc_library(
    name = "libmirage",
    srcs = ["lib/libmirage.dylib"],
    hdrs = glob(["include/libmirage-3.2/**/*.h"]),
    #copts = ["-Iexternal/libmirage/include/libmirage-3.2"],
    includes = ["include/libmirage-3.2"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libmirage_nolib",
    #srcs = ["lib/libmirage.dylib"],
    hdrs = glob(["include/libmirage-3.2/**/*.h"]),
    #copts = ["-Iexternal/libmirage/include/libmirage-3.2"],
    includes = ["include/libmirage-3.2"],
    visibility = ["//visibility:public"],
)
