cc_library(
    name = "util",
    srcs = [
        "constants.cpp",
        "constants.h",
        "util.cpp",
        "util.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//base",
        "//inferui/model:uidump_proto_cpp",
        "//util/recordio",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cpp"],
    copts = ["-DGTEST_USE_OWN_TR1_TUPLE=0"],
    deps = [
        ":util",
        "@gtest//:gtest",
    ],
)
