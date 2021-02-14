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

#include "geomutil.h"

TEST(GeomUtilTest, AngleTest) {
  {
    LineSegment segment(0, 0, 1, 0);
    EXPECT_EQ(segment.GetAngle(), 0);
  }

  {
    LineSegment segment(0, 1, 0, 0);
    EXPECT_EQ(segment.GetAngle(), -90);
  }

  {
    LineSegment segment(1, 0, 0, 0);
    EXPECT_EQ(segment.GetAngle(), 180);
  }

  {
    LineSegment segment(0, 0, 0, 1);
    EXPECT_EQ(segment.GetAngle(), 90);
  }
}

TEST(GeomUtilTest, AngleNaNTest) {
  {
    LineSegment segment(0, 0, 0, 0);
    EXPECT_TRUE(std::isnan(segment.GetAngle()));
  }
}

TEST(GeomUtilTest, IntersectionFalseTest) {
  {
    //below
    LineSegment segment(0, 0, 10, 0);
    LineSegment rectangle(0, 1, 10, 10);
    EXPECT_FALSE(segment.IntersectsLoose(rectangle));
    EXPECT_FALSE(segment.Intersects(rectangle));
  }

  {
    //above
    LineSegment segment(0, 0, 10, 0);
    LineSegment rectangle(0, -1, 10, -10);
    EXPECT_FALSE(segment.IntersectsLoose(rectangle));
    EXPECT_FALSE(segment.Intersects(rectangle));
  }

  {
    //left
    LineSegment segment(0, 0, 1, 0);
    LineSegment rectangle(2, -10, 10, 10);
    EXPECT_FALSE(segment.IntersectsLoose(rectangle));
    EXPECT_FALSE(segment.Intersects(rectangle));
  }

  {
    //right
    LineSegment segment(0, 0, 1, 0);
    LineSegment rectangle(-2, -10, -10, 10);
    EXPECT_FALSE(segment.IntersectsLoose(rectangle));
    EXPECT_FALSE(segment.Intersects(rectangle));
  }

  {
    //inside
    LineSegment segment(0, 0, 1, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_FALSE(segment.IntersectsLoose(rectangle));
    EXPECT_FALSE(segment.Intersects(rectangle));
  }
}

TEST(GeomUtilTest, IntersectionFullTrueTest) {
  {
    //intersects trough left right
    LineSegment segment(-15, 0, 15, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects trough right left
    LineSegment segment(15, 0, -15, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects trough top down
    LineSegment segment(0, -15, 0, 15);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects trough down top
    LineSegment segment(0, 15, 0, -15);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }
}

TEST(GeomUtilTest, IntersectionPartialTrueTest) {
  {
    //intersects partial left
    LineSegment segment(-15, 0, 0, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial left
    LineSegment segment(0, 0, -15, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial right
    LineSegment segment(15, 0, 0, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial right
    LineSegment segment(0, 0, 15, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial top
    LineSegment segment(0, -15, 0, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial top
    LineSegment segment(0, 0, 0, -15);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial down
    LineSegment segment(0, 15, 0, 0);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }

  {
    //intersects partial down
    LineSegment segment(0, 0, 0, 15);
    LineSegment rectangle(-10, -10, 10, 10);
    EXPECT_TRUE(segment.IntersectsLoose(rectangle));
    EXPECT_TRUE(segment.Intersects(rectangle));
  }
}

int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
