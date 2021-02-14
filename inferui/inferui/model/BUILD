load("@protobuf//:protobuf.bzl", "cc_proto_library")
load("@protobuf//:protobuf.bzl", "py_proto_library")

py_proto_library(
    name = "uidump_proto_py",
    srcs = ["uidump.proto"],
    default_runtime = "@protobuf//:protobuf_python",
    protoc = "@protobuf//:protoc",
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "uidump_proto_cpp",
    srcs = ["uidump.proto"],
    default_runtime = "@protobuf//:protobuf",
    protoc = "@protobuf//:protoc",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "model",
    srcs = [
        "constraint_model.cpp",
        "constraint_model.h",
        "constraints.cpp",
        "constraints.h",
        "model.cpp",
        "model.h",
        "syn_helper.cpp",
        "syn_helper.h",
        "synthesis.cpp",
        "synthesis.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":uidump_proto_cpp",
        "//base",
        "//inferui/layout_solver:solver",
        "//inferui/model/util",
        "//json:jsoncpp",
        "//util/recordio",
    ],
)

cc_test(
    name = "model_test",
    srcs = ["model_test.cpp"],
    copts = ["-DGTEST_USE_OWN_TR1_TUPLE=0"],
    deps = [
        ":model",
        "@gtest",
    ],
)

cc_binary(
    name = "constraint_main",
    srcs = [
        "constraint_main.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":model",
        ":uidump_proto_cpp",
        "//base",
        "//inferui/layout_solver:solver",
        "//json:jsoncpp",
        "//util/recordio",
    ],
)
