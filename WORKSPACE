workspace(name = "cdmanip")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_skylib",
    type = "tar.gz",
    url = "https://github.com/bazelbuild/bazel-skylib/releases/download/0.8.0/bazel-skylib.0.8.0.tar.gz",
    sha256 = "2ef429f5d7ce7111263289644d233707dba35e39696377ebab8b0bc701f7818e",
)

http_archive(
    name = "abseil",
    sha256 = "0b62fc2d00c2b2bc3761a892a17ac3b8af3578bd28535d90b4c914b0a7460d4e",
    strip_prefix = "abseil-cpp-20190808",
    urls = ["https://github.com/abseil/abseil-cpp/archive/20190808.zip"],
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "b4fdd8e3733cd88dbe71ebbf553d16e536ff0d5eb1fdba689b8fc7821f65878a",
    strip_prefix = "protobuf-3.9.1",
    urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v3.9.1/protobuf-cpp-3.9.1.zip"],
)

git_repository(
    name = "boringssl",
    commit = "7f9017dd3c60047d6fbc0f617d757c763af8867e",
    remote = "https://boringssl.googlesource.com/boringssl",
    shallow_since = "1542843106 +0000",
)

#http_archive(
#    name = "libmirage",
#    sha256 = "f1f2d2b1eaa42f2cb1c6edbeefb4c76031c7f2f6de5d71c702117a075474993f",
#    strip_prefix = "libmirage-3.2.2",
#    urls = ["http://downloads.sourceforge.net/cdemu/libmirage-3.2.2.tar.bz2"],
#    build_file = "all_content.BUILD",
#)
#
#http_archive(
#    name = "pkgconfig",
#    strip_prefix = "pkg-config-0.29.2",
#    sha256 = "6fc69c01688c9458a57eb9a1664c9aba372ccda420a02bf4429fe610e7e7d591",
#    urls = ["https://pkg-config.freedesktop.org/releases/pkg-config-0.29.2.tar.gz"],
#    build_file = "all_content.BUILD",
#)

git_repository(
    name = "googletest",
    remote = "https://github.com/google/googletest.git",
    commit = "90a443f9c2437ca8a682a1ac625eba64e1d74a8a",
    shallow_since = "1565193450 -0400",
    repo_mapping = {"@com_google_absl": "@abseil"},
)

http_archive(
    name = "curl",
    build_file = "@//third_party:curl.BUILD",
    sha256 = "d0393da38ac74ffac67313072d7fe75b1fa1010eb5987f63f349b024a36b7ffb",
    strip_prefix = "curl-7.66.0",
    urls = ["https://curl.haxx.se/download/curl-7.66.0.tar.gz"],
)

http_archive(
    name = "zlib",
    sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
    strip_prefix = "zlib-1.2.11",
    build_file = "@//third_party:zlib.BUILD",
    urls = ["https://zlib.net/zlib-1.2.11.tar.gz"],
)

new_git_repository(
    name = "curlpp",
    remote = "https://github.com/jpbarrette/curlpp.git",
    commit = "8810334c830faa3b38bcd94f5b1ab695a4f05eb9",
    shallow_since = "1529062451 -0400",
    build_file = "@//third_party:curlpp.BUILD",
)

#local_repository(
#    name = "rhutil",
#    path = "/Users/eatnumber1/Sources/rhutil",
#)
git_repository(
    name = "rhutil",
    remote = "https://github.com/eatnumber1/rhutil.git",
    commit = "f097f8f118e24434c4fdaaf64ec18e084543b1d5",
    shallow_since = "1575881701 -0800",
)

git_repository(
    name = "sumparse",
    remote = "https://github.com/eatnumber1/sumparse.git",
    commit = "658e6a1d6059c76780c73ea493449ffa3fbe24dd",
    shallow_since = "1572162572 -0700",
)

git_repository(
    name = "cue2pb",
    remote = "https://github.com/eatnumber1/cue2pb.git",
    commit = "94220dd075b0f23c2b03ee9324f366fe360a3cf2",
    shallow_since = "1572162605 -0700",
)

git_repository(
    name = "dat2pb",
    remote = "https://github.com/eatnumber1/dat2pb.git",
    commit = "7bfa5688eb835645061f7f621ea544b39613968c",
    shallow_since = "1572169509 -0700",
)

git_repository(
    name = "rules_foreign_cc",
    remote = "https://github.com/bazelbuild/rules_foreign_cc.git",
    commit = "6bb0536452eaca3bad20c21ba6e7968d2eda004d",
    shallow_since = "1571839594 +0200",
)

http_archive(
    name = "libxml",
    build_file = "@dat2pb//third_party:libxml.BUILD",
    sha256 = "f63c5e7d30362ed28b38bfa1ac6313f9a80230720b7fb6c80575eeab3ff5900c",
    strip_prefix = "libxml2-2.9.7",
    urls = ["http://xmlsoft.org/sources/libxml2-2.9.7.tar.gz"],
)

http_archive(
    name = "libzip",
    build_file = "@//third_party:libzip.BUILD",
    sha256 = "be694a4abb2ffe5ec02074146757c8b56084dbcebf329123c84b205417435e15",
    strip_prefix = "libzip-1.5.2",
    urls = ["https://libzip.org/download/libzip-1.5.2.tar.gz"],

    #strip_prefix = "libzip-rel-1-5-1",
    #urls = [
    #    # Bazel does not like the official download link at libzip.org,
    #    # so use the GitHub release tag.
    #    "https://github.com/nih-at/libzip/archive/rel-1-5-1.zip",
    #],
)

#http_archive(
#    name = "zlib",
#    build_file = "@com_google_protobuf//third_party:zlib.BUILD",
#    sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
#    strip_prefix = "zlib-1.2.11",
#    urls = ["https://zlib.net/zlib-1.2.11.tar.gz"],
#)

http_archive(
    name = "hash_library",
    build_file = "@//third_party:hash-library.BUILD",
    sha256 = "ac405a57e6b87c6952c3e42cec5395805bc81af05298e11cb41f89a9a7b4ed5d",
    urls = ["https://create.stephan-brumme.com/hash-library/hash-library.zip"]
)

#local_repository(
#    name = "rcrc",
#    path = "/Users/eatnumber1/Sources/rcrc",
#)
git_repository(
    name = "rcrc",
    remote = "https://github.com/eatnumber1/rcrc.git",
    commit = "de23ded7547e626df164f39f63f5a51a14346311",
    shallow_since = "1575608298 -0800",
)

http_archive(
    name = "nlohmann_json",
    strip_prefix = "include",
    sha256 = "541c34438fd54182e9cdc68dd20c898d766713ad6d901fb2c6e28ff1f1e7c10d",
    build_file = "@rhutil//third_party:json.BUILD",
    urls = ["https://github.com/nlohmann/json/releases/download/v3.7.0/include.zip"],
)

load("@rhutil//:deps.bzl", "rhutil_deps")
rhutil_deps()
load("@rcrc//:deps.bzl", "rcrc_deps")
rcrc_deps()
load("@rhutil//rhutil/curl:deps.bzl", "rhutil_curl_deps")
rhutil_curl_deps()
load("@rhutil//rhutil/json:deps.bzl", "rhutil_json_deps")
rhutil_json_deps()
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")
rules_foreign_cc_dependencies(
    native_tools_toolchains = [
        "@rhutil//tools/build_defs:eatnumber1_cmake_toolchain_osx",
        "@rhutil//tools/build_defs:eatnumber1_ninja_toolchain_osx",
    ],
    register_default_tools = False
)
