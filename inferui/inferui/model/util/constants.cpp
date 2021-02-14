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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include "constants.h"

DEFINE_bool(xml_attributes, true, "Type of attributes. If xml_attributes if true and uidump attributes if false.");

const std::vector<std::string> Constants::values_xml = {
    "{http://schemas.android.com/apk/res/android}id",

    //Relative
    "{http://schemas.android.com/apk/res/android}layout_below",
    "{http://schemas.android.com/apk/res/android}layout_above",

    "{http://schemas.android.com/apk/res/android}layout_toLeftOf",
    "{http://schemas.android.com/apk/res/android}layout_toRightOf",

    "{http://schemas.android.com/apk/res/android}layout_alignLeft",
    "{http://schemas.android.com/apk/res/android}layout_alignRight",

    "{http://schemas.android.com/apk/res/android}layout_alignTop",
    "{http://schemas.android.com/apk/res/android}layout_alignBottom",

//Relative Parent
    "{http://schemas.android.com/apk/res/android}layout_alignParentTop",
    "{http://schemas.android.com/apk/res/android}layout_alignParentBottom",

    "{http://schemas.android.com/apk/res/android}layout_alignParentLeft",
    "{http://schemas.android.com/apk/res/android}layout_alignParentRight",

    "{http://schemas.android.com/apk/res/android}layout_alignParentStart",
    "{http://schemas.android.com/apk/res/android}layout_alignParentEnd",

    "{http://schemas.android.com/apk/res/android}layout_centerHorizontal",
    "{http://schemas.android.com/apk/res/android}layout_centerInParent",
    "{http://schemas.android.com/apk/res/android}layout_centerVertical",

    //Constraint Layout
    "{http://schemas.android.com/apk/res-auto}layout_constraintTop_toBottomOf",
    "{http://schemas.android.com/apk/res-auto}layout_constraintBottom_toBottomOf",

    "{http://schemas.android.com/apk/res-auto}layout_constraintTop_toTopOf",
    "{http://schemas.android.com/apk/res-auto}layout_constraintBottom_toTopOf",

    "{http://schemas.android.com/apk/res-auto}layout_constraintLeft_toLeftOf",
    "{http://schemas.android.com/apk/res-auto}layout_constraintLeft_toRightOf",

    "{http://schemas.android.com/apk/res-auto}layout_constraintRight_toRightOf",
    "{http://schemas.android.com/apk/res-auto}layout_constraintRight_toLeftOf",

    "{http://schemas.android.com/apk/res-auto}layout_constraintHorizontal_bias",
    "{http://schemas.android.com/apk/res-auto}layout_constraintVertical_bias",

    "{http://schemas.android.com/apk/res-auto}layout_constraintBaseline_toBaselineOf",

    //Margins
    "{http://schemas.android.com/apk/res/android}layout_marginTop",
    "{http://schemas.android.com/apk/res/android}layout_marginBottom",
    "{http://schemas.android.com/apk/res/android}layout_marginRight",
    "{http://schemas.android.com/apk/res/android}layout_marginLeft",
    "{http://schemas.android.com/apk/res/android}layout_margin",

    //Padding
    "{http://schemas.android.com/apk/res/android}paddingTop",
    "{http://schemas.android.com/apk/res/android}paddingBottom",
    "{http://schemas.android.com/apk/res/android}paddingRight",
    "{http://schemas.android.com/apk/res/android}paddingLeft",
    "{http://schemas.android.com/apk/res/android}padding",
    "{http://schemas.android.com/apk/res/android}paddingStart",
    "{http://schemas.android.com/apk/res/android}paddingEnd",
    "{http://schemas.android.com/apk/res/android}paddingHorizontal",
    "{http://schemas.android.com/apk/res/android}paddingVertical",

    // View properties
    "{http://schemas.android.com/apk/res/android}layout_width",
    "{http://schemas.android.com/apk/res/android}layout_height",

    // Views
    "android.support.constraint.ConstraintLayout",
    "RelativeLayout",
    "android.support.constraint.Guideline",

    // Values
    "match_parent",
    "0dp",

    //Chains
    "{http://schemas.android.com/apk/res-auto}layout_constraintVertical_chainStyle",
    "{http://schemas.android.com/apk/res-auto}layout_constraintHorizontal_chainStyle",

    // Always keep as last entry
    "NO_NAME",
};

const std::vector<std::string> Constants::values_ui = {
    "attributes:id",

    //Relative
    "attributes:layout_below",
    "attributes:layout_above",

    "attributes:layout_toLeftOf",
    "attributes:layout_toRightOf",

    "attributes:layout_alignLeft",
    "attributes:layout_alignRight",

    "attributes:layout_alignTop",
    "attributes:layout_alignBottom",

    //Relative Parent
    "attributes:layout_alignParentTop",
    "attributes:layout_alignParentBottom",

    "attributes:layout_alignParentLeft",
    "attributes:layout_alignParentRight",

    "attributes:layout_alignParentStart",
    "attributes:layout_alignParentEnd",

    "attributes:layout_centerHorizontal",
    "attributes:layout_centerInParent", // both horizontal and vertical
    "attributes:layout_centerVertical",

    //Constraint Layout
    "attributes:layout_constraintTop_toBottomOf",
    "attributes:layout_constraintBottom_toBottomOf",

    "attributes:layout_constraintTop_toTopOf",
    "attributes:layout_constraintBottom_toTopOf",

    "attributes:layout_constraintLeft_toLeftOf",
    "attributes:layout_constraintLeft_toRightOf",

    "attributes:layout_constraintRight_toRightOf",
    "attributes:layout_constraintRight_toLeftOf",

    "attributes:layout_constraintHorizontal_bias",
    "attributes:layout_constraintVertical_bias",

    "attributes:layout_constraintBaseline_toBaselineOf",

    //Margins
    "layout:layout_topMargin",
    "layout:layout_bottomMargin",
    "layout:layout_rightMargin",
    "layout:layout_leftMargin",
    "layout:layout_margin",

    //TODO: check the padding values
    //Padding
    "padding:mPaddingTop",
    "padding:mPaddingBottom",
    "padding:mPaddingRight",
    "padding:mPaddingLeft",
    "NOT_DEFINED",
    "NOT_DEFINED",
    "NOT_DEFINED",
    "NOT_DEFINED",
    "NOT_DEFINED",

    //View Attributes
    "attributes:layout_width",
    "attributes:layout_height",

    // Views
    "android.support.constraint.ConstraintLayout",
    "RelativeLayout",
    "android.support.constraint.Guideline",

    // Values
    "match_parent",
    "0dp",

    //Chains
    "attributes:layout_constraintVertical_chainStyle",
    "attributes:layout_constraintHorizontal_chainStyle",

    // Always keep as last entry
    "NO_NAME",
};

const std::vector<std::string> Constants::values_output = {
    "android:id",

    //Relative
    "android:layout_below",
    "android:layout_above",

    "android:layout_toLeftOf",
    "android:layout_toRightOf",

    "android:layout_alignLeft",
    "android:layout_alignRight",

    "android:layout_alignTop",
    "android:layout_alignBottom",

//Relative Parent
    "android:layout_alignParentTop",
    "android:layout_alignParentBottom",

    "android:layout_alignParentLeft",
    "android:layout_alignParentRight",

    "android:layout_alignParentStart",
    "android:layout_alignParentEnd",

    "android:layout_centerHorizontal",
    "android:layout_centerInParent",
    "android:layout_centerVertical",

    //Constraint Layout
    "app:layout_constraintTop_toBottomOf",
    "app:layout_constraintBottom_toBottomOf",

    "app:layout_constraintTop_toTopOf",
    "app:layout_constraintBottom_toTopOf",

    "app:layout_constraintLeft_toLeftOf",
    "app:layout_constraintLeft_toRightOf",

    "app:layout_constraintRight_toRightOf",
    "app:layout_constraintRight_toLeftOf",

    "app:layout_constraintHorizontal_bias",
    "app:layout_constraintVertical_bias",

    "app:layout_constraintBaseline_toBaselineOf",

    //Margins
    "android:layout_marginTop",
    "android:layout_marginBottom",
    "android:layout_marginRight",
    "android:layout_marginLeft",
    "android:layout_margin",

    //Padding
    "android:paddingTop",
    "android:paddingBottom",
    "android:paddingRight",
    "android:paddingLeft",
    "android:padding",
    "android:paddingStart",
    "android:paddingEnd",
    "android:paddingHorizontal",
    "android:paddingVertical",

    // View properties
    "android:layout_width",
    "android:layout_height",

    // Views
    "android.support.constraint.ConstraintLayout",
    "RelativeLayout",
    "android.support.constraint.Guideline",

    // Values
    "match_parent",
    "0dp",

    //Chains
    "app:layout_constraintVertical_chainStyle",
    "app:layout_constraintHorizontal_chainStyle",

    // Always keep as last entry
    "NO_NAME",
};

const std::vector<Constants::Name> Properties::RELATIVE_CONSTRAINTS = {
    Constants::layout_below,
    Constants::layout_above,

    Constants::layout_toLeftOf,
    Constants::layout_toRightOf,

    Constants::layout_alignLeft,
    Constants::layout_alignRight,

    Constants::layout_alignTop,
    Constants::layout_alignBottom,
//    "attributes:layout_below",
//    "attributes:layout_above",
//
//    "attributes:layout_toLeftOf",
//    "attributes:layout_toRightOf",
//
//    "attributes:layout_alignLeft",
//    "attributes:layout_alignRight",
//
//    "attributes:layout_alignTop",
//    "attributes:layout_alignBottom",
};

const std::vector<Constants::Name> Properties::CONSTRAINT_LAYOUT_CONSTRAINTS = {
    Constants::layout_constraintTop_toBottomOf,
    Constants::layout_constraintBottom_toBottomOf,

    Constants::layout_constraintTop_toTopOf,
    Constants::layout_constraintBottom_toTopOf,

    Constants::layout_constraintLeft_toLeftOf,
    Constants::layout_constraintLeft_toRightOf,

    Constants::layout_constraintRight_toRightOf,
    Constants::layout_constraintRight_toLeftOf,

//    Constants::layout_constraintStart_toEndOf,
//    Constants::layout_constraintEnd_toStartOf,
};

const std::vector<Constants::Name> Properties::LEFT_CONSTRAINT_LAYOUT_CONSTRAINTS = {
    Constants::layout_constraintLeft_toLeftOf,
    Constants::layout_constraintLeft_toRightOf,
};

const std::vector<Constants::Name> Properties::RIGHT_CONSTRAINT_LAYOUT_CONSTRAINTS = {
    Constants::layout_constraintRight_toRightOf,
    Constants::layout_constraintRight_toLeftOf,
};

const std::vector<Constants::Name> Properties::TOP_CONSTRAINT_LAYOUT_CONSTRAINTS = {
    Constants::layout_constraintTop_toTopOf,
    Constants::layout_constraintTop_toBottomOf,
};

const std::vector<Constants::Name> Properties::BOTTOM_CONSTRAINT_LAYOUT_CONSTRAINTS = {
    Constants::layout_constraintBottom_toTopOf,
    Constants::layout_constraintBottom_toBottomOf,
};

//const std::vector<std::pair<Constants::Name, Constants::Name>> Properties::CONSTRAINT_LAYOUT_ATTRIBUTES = {
//    {Constants::layout_constraintLeft_toLeftOf, Constants::NO_NAME},
//    {Constants::layout_constraintLeft_toRightOf, Constants::NO_NAME},
//
//    {Constants::layout_constraintRight_toRightOf, Constants::NO_NAME},
//    {Constants::layout_constraintRight_toLeftOf, Constants::NO_NAME},
//
//    {Constants::layout_constraintLeft_toLeftOf, Constants::layout_constraintRight_toRightOf},
//    {Constants::layout_constraintLeft_toLeftOf, Constants::layout_constraintRight_toLeftOf},
//
//    {Constants::layout_constraintLeft_toRightOf, Constants::layout_constraintRight_toRightOf},
//    {Constants::layout_constraintLeft_toRightOf, Constants::layout_constraintRight_toLeftOf},
//
//    {Constants::layout_constraintTop_toTopOf, Constants::NO_NAME},
//    {Constants::layout_constraintTop_toBottomOf, Constants::NO_NAME},
//
//    {Constants::layout_constraintBottom_toTopOf, Constants::NO_NAME},
//    {Constants::layout_constraintBottom_toBottomOf, Constants::NO_NAME},
//
//    {Constants::layout_constraintTop_toTopOf, Constants::layout_constraintBottom_toTopOf},
//    {Constants::layout_constraintTop_toTopOf, Constants::layout_constraintBottom_toBottomOf},
//
//    {Constants::layout_constraintTop_toBottomOf, Constants::layout_constraintBottom_toTopOf},
//    {Constants::layout_constraintTop_toBottomOf, Constants::layout_constraintBottom_toBottomOf},
//};

const std::vector<Constants::Name> Properties::RELATIVE_PARENT_CONSTRAINTS = {
    Constants::layout_alignParentTop,
    Constants::layout_alignParentBottom,

    Constants::layout_alignParentLeft,
    Constants::layout_alignParentRight,

    Constants::layout_alignParentStart,
    Constants::layout_alignParentEnd,

    Constants::layout_centerHorizontal,
    Constants::layout_centerInParent, // both horizontal and vertical
    Constants::layout_centerVertical,
};

const std::vector<Constants::Name>& Constants::ViewProperties(Constants::Name name) {
  switch (name) {
    case ConstraintLayout:
      return Properties::CONSTRAINT_LAYOUT_CONSTRAINTS;
    case RelativeLayout:
      return Properties::RELATIVE_CONSTRAINTS;
    default:
      LOG(FATAL) << "Unknown properties for name: '" << name << "'";
  }
}
