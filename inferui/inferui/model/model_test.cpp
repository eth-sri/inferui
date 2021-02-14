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

#include "model.h"

TEST(ModelTest, AttrSize) {
  AttrSizeModel model;

  View a(0, 0, 10, 10, "0", 0);
  View b(0, 0, 10, 10, "1", 1);

  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &a, &b);
    EXPECT_EQ(model.AttrProb(attr, {}), 0.03);
  }
}

TEST(ModelTest, OrientationModel) {
  OrientationModel model;

  float RIGHT = 0;
  float LEFT = 180;
  float UP = 90;
  float DOWN = -90;

  View content_frame = View(-5, -5, 5, 5, "Root", 0);
  View tr = View(0, 0, 1, 1, "Button", 1);
  View tl = View(-1, 0, 0, 1, "Button", 2);
  View br = View(0, 2, 1, 3, "Button", 3);
  View bl = View(-1, 2, 0, 3, "Button", 4);
  View bfull = View(-1, 2, 1, 3, "Button", 5);
  View tfull = View(-1, 0, 0, 1, "Button", 6);
  View tm = View(-0.5, 0, 0.5, 1, "Button", 7);
  View bm = View(-0.5, 2, 0.5, 3, "Button", 8);

  std::vector<View> nodes = {
      content_frame,
      tr, tl,
      br, bl,
      bfull, tfull,
      tm, bm
  };

  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tr, &tl);
    EXPECT_EQ(model.AttrValue(attr, nodes), LEFT);
  }
  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tr, &br);
    EXPECT_EQ(model.AttrValue(attr, nodes), UP);
  }

  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tl, &tr);
    EXPECT_EQ(model.AttrValue(attr, nodes), RIGHT);
  }
  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tl, &bl);
    EXPECT_EQ(model.AttrValue(attr, nodes), UP);
  }

  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &br, &tr);
    EXPECT_EQ(model.AttrValue(attr, nodes), DOWN);
  }
  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &br, &bl);
    EXPECT_EQ(model.AttrValue(attr, nodes), LEFT);
  }

  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &bl, &tl);
    EXPECT_EQ(model.AttrValue(attr, nodes), DOWN);
  }
  {
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &bl, &br);
    EXPECT_EQ(model.AttrValue(attr, nodes), RIGHT);
  }

  {
    Attribute attr(ConstraintType::L2LxR2R, ViewSize::FIXED, 0, &tm, &bm, &bm);
    EXPECT_EQ(model.AttrValue(attr, nodes), UP);
  }
  {
    Attribute attr(ConstraintType::L2LxR2R, ViewSize::FIXED, 0, &tm, &bfull, &bfull);
    EXPECT_EQ(model.AttrValue(attr, nodes), UP);
  }
}


TEST(ModelTest, AnchorTest) {
  View content_frame = View(-5, -5, 5, 5, "Root", 0);
  View tr = View(0, 0, 1, 1, "Button", 1);
  View tl = View(-1, 0, 0, 1, "Button", 2);
  View br = View(0, 1, 1, 2, "Button", 3);
  View bl = View(-1, 1, 0, 2, "Button", 4);
  View bfull = View(-1, 1, 1, 2, "Button", 5);
  View tfull = View(-1, 1, 0, 1, "Button", 6);
  View tm = View(-0.5, 0, 0.5, 1, "Button", 7);
  View bm = View(-0.5, 1, 0.5, 2, "Button", 8);

  {
    for (Orientation orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
//      View tl = View(-1, 0, 0, 1, "Button", 2);
      EXPECT_TRUE(content_frame.IsAnchored(orientation));
      EXPECT_FALSE(tr.IsAnchored(orientation));
      EXPECT_FALSE(tl.IsAnchored(orientation));
      EXPECT_FALSE(br.IsAnchored(orientation));
      EXPECT_FALSE(bl.IsAnchored(orientation));
    }
  }

  {
//    View tl = View(-1, 0, 0, 1, "Button", 2);
    Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tl, &content_frame);
    tl.ApplyAttribute(Orientation::HORIZONTAL, attr);

    EXPECT_TRUE(content_frame.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_TRUE(tl.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_FALSE(tl.IsAnchored(Orientation::VERTICAL));
  }

  {
//    View tl = View(-1, 0, 0, 1, "Button", 2);
    Attribute attr(ConstraintType::L2LxR2R, ViewSize::FIXED, 0, &tl, &content_frame, &tr);
    tl.ApplyAttribute(Orientation::HORIZONTAL, attr);

    EXPECT_TRUE(content_frame.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_FALSE(tl.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_FALSE(tr.IsAnchored(Orientation::HORIZONTAL));
  }

  {

    {
      Attribute attr(ConstraintType::L2LxR2R, ViewSize::FIXED, 0, &br, &bl, &bl);
      br.ApplyAttribute(Orientation::HORIZONTAL, attr);
    }
    {
      Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &bl, &content_frame);
      bl.ApplyAttribute(Orientation::HORIZONTAL, attr);
    }

    EXPECT_TRUE(content_frame.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_TRUE(bl.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_TRUE(br.IsAnchored(Orientation::HORIZONTAL));
  }

}

TEST(ModelTest, AnchorTest2) {
  View content_frame = View(-5, -5, 5, 5, "Root", 0);
  View tr = View(0, 0, 1, 1, "Button", 1);
  View tl = View(-1, 0, 0, 1, "Button", 2);
  View br = View(0, 1, 1, 2, "Button", 3);
  View bl = View(-1, 1, 0, 2, "Button", 4);
  View bfull = View(-1, 1, 1, 2, "Button", 5);
  View tfull = View(-1, 1, 0, 1, "Button", 6);
  View tm = View(-0.5, 0, 0.5, 1, "Button", 7);
  View bm = View(-0.5, 1, 0.5, 2, "Button", 8);


  {

    {
      Attribute attr(ConstraintType::L2LxR2R, ViewSize::FIXED, 0, &br, &tl, &tr);
      br.ApplyAttribute(Orientation::HORIZONTAL, attr);
    }
    {
      Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tr, &tl);
      tr.ApplyAttribute(Orientation::HORIZONTAL, attr);
    }
    {
      Attribute attr(ConstraintType::L2L, ViewSize::FIXED, 0, &tl, &content_frame);
      tl.ApplyAttribute(Orientation::HORIZONTAL, attr);
    }

    EXPECT_TRUE(content_frame.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_TRUE(br.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_TRUE(tr.IsAnchored(Orientation::HORIZONTAL));
    EXPECT_TRUE(tl.IsAnchored(Orientation::HORIZONTAL));
  }

}


int main(int argc, char **argv) {
  google::InstallFailureSignalHandler();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
