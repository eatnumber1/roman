cc_library(
    name = "curlpp",
    srcs = (
        glob(["include/curlpp/internal/**"]) +
        glob(["include/utilspp/**"]) +
        glob(["src/curlpp/**"])
    ),
    hdrs = glob(
        include = ["include/curlpp/**"],
        exclude = ["include/curlpp/internal/**"],
    ),
    visibility = ["//visibility:public"],
    includes = ["include/"],
    deps = [
        "@curl//:curl",
    ]
)
