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

#include "constraints.h"

DEFINE_bool(uniform_probability, false, "Whether all attributes have uniform probability (baseline)");

float CenterMargin(const ConstraintType& type, const View& a, const View& l, const View& r, float bias) {
  switch (type) {
    case ConstraintType::L2LxR2R:
      return ((a.xleft + a.xright) * bias) - ((l.xleft + r.xright) * bias);
    case ConstraintType::L2RxR2L:
      return ((a.xleft + a.xright) * bias) - ((l.xright + r.xleft) * bias);
    case ConstraintType::L2LxR2L:
      return ((a.xleft + a.xright) * bias) - ((l.xleft + r.xleft) * bias);
    case ConstraintType::L2RxR2R:
      return ((a.xleft + a.xright) * bias) - ((l.xright + r.xright) * bias);

    case ConstraintType::T2TxB2T:
      return ((a.ytop + a.ybottom) * bias) - ((l.ytop + r.ytop) * bias);
    case ConstraintType::T2TxB2B:
      return ((a.ytop + a.ybottom) * bias) - ((l.ytop + r.ybottom) * bias);
    case ConstraintType::T2BxB2T:
      return ((a.ytop + a.ybottom) * bias) - ((l.ybottom + r.ytop) * bias);
    case ConstraintType::T2BxB2B:
      return ((a.ytop + a.ybottom) * bias) - ((l.ybottom + r.ybottom) * bias);
    default:
      LOG(FATAL) << "Not a relational constraint: " << ConstraintTypeStr(type);
  }
}

float RelationalMargin(const ConstraintType& type, const View& a, const View& b) {
  switch (type) {
    case ConstraintType::T2T:
      return a.ytop - b.ytop;
    case ConstraintType::T2B:
      return a.ytop - b.ybottom;
    case ConstraintType::B2T:
      return b.ytop - a.ybottom;
    case ConstraintType::B2B:
      return b.ybottom - a.ybottom;

    case ConstraintType::L2L:
      return a.xleft - b.xleft;
    case ConstraintType::L2R:
      return a.xleft - b.xright;
    case ConstraintType::R2L:
      return b.xleft - a.xright;
    case ConstraintType::R2R:
      return b.xright - a.xright;
    default:
      LOG(FATAL) << "Not a relational constraint: " << ConstraintTypeStr(type);
  }
}
