
cc_library(name="subprocess",
           srcs = [ "subprocess.cpp",
                    "subprocess.h",
                  ],
           deps = [ "//base" ],
           visibility = ["//visibility:public"])

cc_test(name = "subprocess_test",
        srcs = ["subprocess_test.cpp"],
        deps = [":subprocess",
                "@gtest//:gtest"])
