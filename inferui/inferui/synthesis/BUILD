cc_library(
    name = "z3model",
    srcs = [
        "z3inference.cpp",
        "z3inference.h",
    ],
    linkopts = [
        "-lz3",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//base",
        "//base:z3base",
        "//inferui/eval:eval_app_util",
        "//inferui/layout_solver:solver",
        "//inferui/model",
    ],
)
