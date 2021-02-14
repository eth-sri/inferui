cc_binary(
    name = "inferui_baseline",
    srcs = [
        "inferui_baseline.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//inferui/datasets:dataset_util",
        "//inferui/eval:eval_util",
        "//inferui/layout_solver:solver",
        "//inferui/model",
        "//inferui/synthesis:z3model",
    ],
)

cc_binary(
    name = "nogis_iterative",
    srcs = [
        "nogis_iterative.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//inferui/eval:eval_util",
        "//inferui/layout_solver:solver",
        "//inferui/datasets:dataset_util",
        "//inferui/model",
        "//inferui/model/util",
        "//inferui/synthesis:z3model",
        "//util/recordio",
        "//json:jsonrpc",
    ],
)
