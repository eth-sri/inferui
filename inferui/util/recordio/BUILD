load("@protobuf//:protobuf.bzl", "cc_proto_library")
load("@protobuf//:protobuf.bzl", "py_proto_library")

cc_library(
    name = "recordio",
    srcs = [
        "recordio.cpp",
        "recordio.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//util/zstream",
        "@protobuf",
    ],
)

py_library(
    name = "recordio_py",
    srcs = ["recordio.py"],
    visibility = ["//visibility:public"],
    deps = ["@protobuf//:protobuf_python"],
)
