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

#ifndef CC_SYNTHESIS_CONSTANTS_H
#define CC_SYNTHESIS_CONSTANTS_H

#include <gflags/gflags_declare.h>
#include <glog/logging.h>
#include <array>

DECLARE_bool(xml_attributes);

class Constants {
public:

  enum Type {
    INPUT_UI = 0,
    INPUT_XML,
    OUTPUT_XML
  };

  enum Name {
    ID,

    //Relative
    layout_below,
    layout_above,

    layout_toLeftOf,
    layout_toRightOf,

    layout_alignLeft,
    layout_alignRight,

    layout_alignTop,
    layout_alignBottom,

    //Relative Parent
    layout_alignParentTop,
    layout_alignParentBottom,

    layout_alignParentLeft,
    layout_alignParentRight,

    layout_alignParentStart,
    layout_alignParentEnd,

    layout_centerHorizontal,
    layout_centerInParent,
    layout_centerVertical,

    //Constraint
    layout_constraintTop_toBottomOf,
    layout_constraintBottom_toBottomOf,

    layout_constraintTop_toTopOf,
    layout_constraintBottom_toTopOf,

    layout_constraintLeft_toLeftOf,
    layout_constraintLeft_toRightOf,

    layout_constraintRight_toRightOf,
    layout_constraintRight_toLeftOf,

    layout_constraintHorizontal_bias,
    layout_constraintVertical_bias,

    layout_constraintBaseline_toBaselineOf,
//    layout_constraintStart_toEndOf,
//    layout_constraintEnd_toStartOf,

    //Margins
    layout_marginTop,
    layout_marginBottom,
    layout_marginRight,
    layout_marginLeft,
    layout_margin,

    //Padding
    paddingTop,
    paddingBottom,
    paddingRight,
    paddingLeft,
    padding,
    paddingStart,
    paddingEnd,
    paddingHorizontal,
    paddingVertical,

    // View Properties
    layout_width,
    layout_height,

    //Views
    ConstraintLayout,
    RelativeLayout,
    Guideline,

    // Values
    MATCH_PARENT,
    MATCH_CONSTRAINT,

    //Chains
    layout_constraintVertical_chainStyle,
    layout_constraintHorizontal_chainStyle,

    NO_NAME,
  };

  static const std::string name(Name name) {
//    if (FLAGS_xml_attributes) {
//      return values_xml[static_cast<int>(name)];
//    } else {
//      return values_ui[static_cast<int>(name)];
//    }
    return Constants::name(name, static_cast<Type>(FLAGS_xml_attributes));
  }

  static const std::string name(Name name, Type type) {
    switch (type) {
      case INPUT_XML:
        return values_xml[static_cast<int>(name)];
      case INPUT_UI:
        return values_ui[static_cast<int>(name)];
      case OUTPUT_XML:
        return values_output[static_cast<int>(name)];
    }
    LOG(FATAL) << "Unknown type: " << type << "'";
  }

  static const std::vector<Constants::Name>& ViewProperties(Name name);

private:
  static const std::vector<std::string> values_xml;
  static const std::vector<std::string> values_ui;
  static const std::vector<std::string> values_output;
};

class Properties {
public:

//private:
//  static const std::string ID;
  static const std::vector<Constants::Name> RELATIVE_CONSTRAINTS;
  static const std::vector<Constants::Name> RELATIVE_PARENT_CONSTRAINTS;

  static const std::vector<Constants::Name> CONSTRAINT_LAYOUT_CONSTRAINTS;

  static const std::vector<Constants::Name> LEFT_CONSTRAINT_LAYOUT_CONSTRAINTS;
  static const std::vector<Constants::Name> RIGHT_CONSTRAINT_LAYOUT_CONSTRAINTS;
  static const std::vector<Constants::Name> TOP_CONSTRAINT_LAYOUT_CONSTRAINTS;
  static const std::vector<Constants::Name> BOTTOM_CONSTRAINT_LAYOUT_CONSTRAINTS;

//  static const std::vector<std::pair<Constants::Name, Constants::Name>> CONSTRAINT_LAYOUT_ATTRIBUTES;

//  static const std::unordered_map<Constants::Name, ConstraintType> ATTRIBUTE_TO_TYPE;
};

namespace std {
  template <> struct hash<Constants::Name> {
    size_t operator()(const Constants::Name& x) const {
      return static_cast<int>(x);
    }
  };
}

#endif //CC_SYNTHESIS_CONSTANTS_H
