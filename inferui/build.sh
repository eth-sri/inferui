#!/bin/bash

bazel build -c opt //... --cxxopt="-fopenmp" --linkopt="-fopenmp" --cxxopt="-DGTEST_HAS_TR1_TUPLE=0"
