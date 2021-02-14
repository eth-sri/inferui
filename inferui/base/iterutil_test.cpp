/*
   Copyright 2018 Software Reliability Lab, ETH Zurich

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "gtest/gtest.h"
#include "glog/logging.h"

#include "iterutil.h"
#include "iterutil_test.h"

TEST(IterUtilTest, EmptyIntInput) {
  std::vector<int> a{};
  std::vector<std::vector<int>> data{ a };
  MultiSortedIterator<int> it(data);
  MultiSortedIterator<int> end(data, true);

  std::vector<int> output = GetOutput(MultiSortedIterator<int>(data), MultiSortedIterator<int>(data, true));
  EXPECT_EQ(a, output);
}

TEST(IterUtilTest, SingleIntInput) {
  std::vector<int> a{11, 5, 2};
  std::vector<std::vector<int>> data{ a };
  MultiSortedIterator<int> it(data);
  MultiSortedIterator<int> end(data, true);

  std::vector<int> output = GetOutput(MultiSortedIterator<int>(data), MultiSortedIterator<int>(data, true));
  EXPECT_EQ(a, output);
}

TEST(IterUtilTest, DoubleIntInput) {
  std::vector<int> a{11, 5, 2};
  std::vector<int> b{11, 6, 4};
  std::vector<std::vector<int>> data{ a, b };
  MultiSortedIterator<int> it(data);
  MultiSortedIterator<int> end(data, true);

  std::vector<int> output = GetOutput(MultiSortedIterator<int>(data), MultiSortedIterator<int>(data, true));
  std::vector<int> expected_output({11, 11, 6, 5, 4, 2});
  EXPECT_EQ(expected_output, output);
}

TEST(IterUtilTest, DoubleIntInput2) {
  std::vector<int> a{2};
  std::vector<int> b{11, 6, 4, 1};
  std::vector<std::vector<int>> data{ a, b };
  MultiSortedIterator<int> it(data);
  MultiSortedIterator<int> end(data, true);

  std::vector<int> output = GetOutput(MultiSortedIterator<int>(data), MultiSortedIterator<int>(data, true));
  std::vector<int> expected_output({11, 6, 4, 2, 1});
  EXPECT_EQ(expected_output, output);
}

TEST(IterUtilTest, DoubleFloatInput) {
  std::vector<float> a{2.1};
  std::vector<float> b{11.5, 6.4, 4.3, 1.2};
  std::vector<std::vector<float>> data{ a, b };
  MultiSortedIterator<float> it(data);
  MultiSortedIterator<float> end(data, true);

  std::vector<float> output = GetOutput(MultiSortedIterator<float>(data), MultiSortedIterator<float>(data, true));
  std::vector<float> expected_output({11.5, 6.4, 4.3, 2.1, 1.2});
  EXPECT_EQ(expected_output, output);
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
