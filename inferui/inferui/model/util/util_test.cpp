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

#include "util.h"

void testForValue(std::string value, double expected, bool has_value) {
  EXPECT_EQ(has_value, ValueParser::hasValue(value));
  if (has_value) {
    EXPECT_EQ(expected, ValueParser::parseValue(value));
  }
}

TEST(ModelTest, ValueParser) {
  testForValue("0.0dp", 0, true);
  testForValue("0dp", 0, true);
  testForValue("10dp", 10, true);
  testForValue("42dp", 42, true);

  testForValue("42dip", 42, true);

  testForValue("-10dp", -10, true);

  testForValue("10.5dp", 10.5, true);

  testForValue("@dimen/dimen_8_dp", 8, true);

  testForValue("@dimen/dp8", 8, true);

  testForValue("@dimen/dp_10", 10, true);

  testForValue("@dimen/padding_normal", 0, false);

  testForValue("10", 10, true);

  testForValue("a10", 0, false);
  testForValue("10a", 0, false);
}

TEST(ModelTest, ValueIDParser) {
  EXPECT_EQ(ValueParser::parseID("@id/test"), "id/test");
  EXPECT_EQ(ValueParser::parseID("@id/test42"), "id/test42");

  EXPECT_EQ(ValueParser::parseID("@+id/test"), "id/test");
  EXPECT_EQ(ValueParser::parseID("@+id/test42"), "id/test42");

  EXPECT_EQ(ValueParser::parseID("@android:id/title"), "android:id/title");
  EXPECT_EQ(ValueParser::parseID("@+android:id/title"), "android:id/title");
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
