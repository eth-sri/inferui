cc_library(
    name = "eval_util",
    srcs = [
        "eval_util.cpp",
        "eval_util.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":eval_app_util",
        "//inferui/layout_solver:solver",
        "//inferui/model",
        "//inferui/model/util",
        "//inferui/synthesis:z3model",
    ],
)

cc_library(
    name = "eval_app_util",
    srcs = [
        "eval_app_util.cpp",
        "eval_app_util.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//inferui/model",
        "//inferui/model/util",
    ],
)
