
cc_library(name = "work_queue",
           hdrs = [ "work_queue.h" ],
           deps = [ "//base" ],
           visibility = ["//visibility:public"])

cc_test(name = "work_queue_test",
        srcs = [ "work_queue_test.cpp" ],
        linkopts = ["-lm"],                #  Math library
        deps = [ ":work_queue",
                 "@gtest//:gtest" ])
