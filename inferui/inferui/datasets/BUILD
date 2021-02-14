cc_library(
    name = "dataset_util",
    srcs = [
        "dataset_util.cpp",
        "dataset_util.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//inferui/eval:eval_util",
        "//inferui/model",
        "//inferui/model/util",
        "//inferui/synthesis:z3model",
    ],
)

cc_binary(
    name = "scale_dataset_syn",
    srcs = [
        "scale_dataset_syn.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":dataset_util",
        "//inferui/eval:eval_util",
        "//inferui/layout_solver:solver",
        "//inferui/model",
        "//inferui/synthesis:z3model",
        "//util/recordio",
    ],
)

cc_binary(
    name = "scale_dataset_from_layout",
    srcs = [
        "scale_dataset_from_layout.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":dataset_util",
        "//inferui/eval:eval_util",
        "//inferui/layout_solver:solver",
        "//inferui/model",
        "//inferui/synthesis:z3model",
        "//util/recordio",
    ],
)

cc_binary(
    name = "iterative_dataset_gen",
    srcs = [
        "iterative_dataset_gen.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":dataset_util",
        "//inferui/eval:eval_util",
        "//inferui/layout_solver:solver",
        "//inferui/model",
        "//inferui/synthesis:z3model",
        "//util/recordio",
    ],
)
