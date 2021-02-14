#!/bin/bash

bazel build -c dbg //... --cxxopt="-DGTEST_HAS_TR1_TUPLE=0"
