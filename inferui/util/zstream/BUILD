licenses(["notice"])

cc_library(name = "zstream",
           hdrs = [
               "strict_fstream.h",
               "zstream.h",
           ],
           deps = [
               "@zlib_repo//:zlib",
           ],
           visibility = ["//visibility:public"])

cc_test(name = "zstream_test",
        srcs = ["zstream_test.cpp"],
        deps = [ "//base",
                 ":zstream",
                 "@gtest//:gtest",
               ])