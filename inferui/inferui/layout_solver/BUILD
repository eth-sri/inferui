cc_library(
    name = "solver",
    srcs = [
        "solver.cpp",
        "solver.h",
    ],
    linkopts = [
        "-lcurl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//json:jsoncpp",
    ],
)

cc_binary(
    name = "solver_sample",
    srcs = [
        "solver_sample.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//base",
        "//inferui/eval:eval_app_util",
        "//inferui/eval:eval_util",
        "//inferui/model",
        "//json:jsonrpc",
    ],
)
