cc_library(
    name = "base",
    srcs = [
        "base.cpp",
        "base.h",
        "containerutil.h",
        "counter.h",
        "fileutil.cpp",
        "fileutil.h",
        "geomutil.cpp",
        "geomutil.h",
        "github.cpp",
        "github.h",
        "iterutil.cpp",
        "iterutil.h",
        "maputil.h",
        "range.h",
        "readerutil.h",
        "serializeutil.cpp",
        "serializeutil.h",
        "simple_histogram.h",
        "stringprintf.cpp",
        "stringprintf.h",
        "stringset.cpp",
        "stringset.h",
        "strutil.cpp",
        "strutil.h",
        "termcolor.cpp",
        "termcolor.h",
        "treeprinter.h",
    ],
    copts = [
        "-fopenmp",
    ],
    linkopts = [
        "-lgflags",
        "-lglog",
        "-lpthread",
        "-lstdc++fs",
        "-lcrypto",  #openssl
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "geomutil_test",
    srcs = ["geomutil_test.cpp"],
    copts = ["-DGTEST_USE_OWN_TR1_TUPLE=0"],
    deps = [
        ":base",
        "@gtest//:gtest",
    ],
)

cc_test(
    name = "iterutil_test",
    srcs = [
        "iterutil_test.cpp",
        "iterutil_test.h",
    ],
    copts = ["-DGTEST_USE_OWN_TR1_TUPLE=0"],
    deps = [
        ":base",
        "@gtest//:gtest",
    ],
)

cc_library(
    name = "z3base",
    srcs = [
        "z3_base.cpp",
        "z3_base.h",
    ],
    linkopts = [
        "-lz3",
    ],
    visibility = ["//visibility:public"],
    deps = [
    ],
)
