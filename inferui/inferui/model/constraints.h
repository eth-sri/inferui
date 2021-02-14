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

#ifndef CC_SYNTHESIS_CONSTRAINTS_H
#define CC_SYNTHESIS_CONSTRAINTS_H


#include "inferui/model/model.h"
#include "base/containerutil.h"

DECLARE_bool(uniform_probability);

float CenterMargin(const ConstraintType& type, const View& a, const View& l, const View& r, float bias = 0.5);
float RelationalMargin(const ConstraintType& type, const View& a, const View& b);

//template<class Model>
class ConstraintGenerator {
public:

  ConstraintGenerator() {
  }

  template <class Callback>
  void GenFixedSizeCenteringConstraints(const Orientation& orientation, View& src, std::vector<View>& views, const Callback& cb) const {
    std::vector<ConstraintType> types = (orientation == Orientation::HORIZONTAL)
                                        ? std::vector<ConstraintType>{ ConstraintType::L2LxR2L, ConstraintType::L2LxR2R, ConstraintType::L2RxR2L, ConstraintType::L2RxR2R }
                                        : std::vector<ConstraintType>{ ConstraintType::T2TxB2T, ConstraintType::T2TxB2B, ConstraintType::T2BxB2T, ConstraintType::T2BxB2B };

    Product(views, [&types, &views, &cb, &src, this](View& l, View& r) {
      if (l == src || r == src) return;

      for (const ConstraintType& type : types) {
        float margin = CenterMargin(type, src, l, r);
        if (std::fabs(margin) < 0.5) {
          margin = 0;
        }
        // When both anchors are the same the margin is ignored by the layout solver
        if (l == r && margin != 0 &&
            (type == ConstraintType::L2LxR2L || type == ConstraintType::L2RxR2R || type == ConstraintType::T2TxB2T || type == ConstraintType::T2BxB2B)) {
          continue;
        }

        cb(Attribute(type, ViewSize::FIXED, (margin > 0) ? margin * 2 : 0, (margin < 0) ? margin * -2 : 0, &src, &l, &r));
      }
    });
  }

  template <class Callback>
  void GenFixedSizeRelationalConstraints(const Orientation& orientation, View& src, std::vector<View>& views, const Callback& cb) const {
    std::vector<ConstraintType> types = (orientation == Orientation::HORIZONTAL)
                                        ? std::vector<ConstraintType>{ ConstraintType::L2L, ConstraintType::L2R, ConstraintType::R2L, ConstraintType::R2R }
                                        : std::vector<ConstraintType>{ ConstraintType::T2T, ConstraintType::T2B, ConstraintType::B2T, ConstraintType::B2B };
    for (View& view : views) {
      if (view == src) continue;

      for (const ConstraintType& type : types) {
        if (view.is_content_frame() && (
                                           type == ConstraintType::T2B || type == ConstraintType::B2T ||
                                           type == ConstraintType::L2R || type == ConstraintType::R2L
                                       )) {
          continue;
        }

        float margin = RelationalMargin(type, src, view);
        if (margin < 0) continue;

        cb(std::move(Attribute(type, ViewSize::FIXED, margin, &src, &view)));
      }
    }
  }

  template <class Callback>
  void GenMatchParentCenteringConstraints(const Orientation& orientation, View& src, std::vector<View>& views, const Callback& cb) const {
    // fill_parent always matches the parent and ignores any constraints
    // Therefore the only constraints that make sense and match the behaviour are centering the constraints on parent

    ConstraintType type = (orientation == Orientation::HORIZONTAL) ? ConstraintType::L2LxR2R : ConstraintType::T2TxB2B;

    View& parent = views.at(0);
    CHECK(parent.is_content_frame());
    const auto types = SplitCenterAnchor(type);
    float marginStart = RelationalMargin(types.first, src, parent);
    float marginEnd = RelationalMargin(types.second, src, parent);

    if (marginStart < 0 || marginEnd < 0) {
      return;
    }

    cb(Attribute(type, ViewSize::MATCH_PARENT, marginStart, marginEnd, &src, &parent, &parent));
  }

  template <class Callback>
  void GenMatchConstraintCenteringConstraints(const Orientation& orientation, View& src, std::vector<View>& views, const Callback& cb) const {
    // match_constraint needs two constraints to specify the view dimensions

    std::vector<ConstraintType> types = (orientation == Orientation::HORIZONTAL)
                                        ? std::vector<ConstraintType>{ ConstraintType::L2LxR2L, ConstraintType::L2LxR2R, ConstraintType::L2RxR2L, ConstraintType::L2RxR2R }
                                        : std::vector<ConstraintType>{ ConstraintType::T2TxB2T, ConstraintType::T2TxB2B, ConstraintType::T2BxB2T, ConstraintType::T2BxB2B };

    Product(views, [&types, &views, &cb, &src, this](View& l, View& r) {
      if (l == src || r == src) return;

      for (const ConstraintType& type : types) {
        const auto types = SplitCenterAnchor(type);
        float marginStart = RelationalMargin(types.first, src, l);
        float marginEnd = RelationalMargin(types.second, src, r);

        if (marginStart < 0 || marginEnd < 0) {
          continue;
        }

        cb(Attribute(type, ViewSize::MATCH_CONSTRAINT, marginStart, marginEnd, &src, &l, &r));
      }
    });
  }

private:
//  const Model* model_;
};


#endif //CC_SYNTHESIS_CONSTRAINTS_H
