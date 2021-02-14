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

#include "model.h"
#include <fstream>
#include <string>
#include <iostream>



const std::unordered_map<Constants::Name, ConstraintType> AttrModel::ATTRIBUTE_TO_TYPE = {
    //Relative Layout
    {Constants::layout_below, ConstraintType::T2B},
    {Constants::layout_above, ConstraintType::B2T},

    {Constants::layout_toLeftOf, ConstraintType::R2L},
    {Constants::layout_toRightOf, ConstraintType::L2R},

    {Constants::layout_alignLeft, ConstraintType::L2L},
    {Constants::layout_alignRight, ConstraintType::R2R},

    {Constants::layout_alignTop, ConstraintType::T2T},
    {Constants::layout_alignBottom, ConstraintType::B2B},

    //Constraint Layout
    {Constants::layout_constraintTop_toBottomOf, ConstraintType::T2B},
    {Constants::layout_constraintBottom_toBottomOf, ConstraintType::B2B},

    {Constants::layout_constraintTop_toTopOf, ConstraintType::T2T},
    {Constants::layout_constraintBottom_toTopOf, ConstraintType::B2T},

    {Constants::layout_constraintLeft_toLeftOf, ConstraintType::L2L},
    {Constants::layout_constraintLeft_toRightOf, ConstraintType::L2R},

    {Constants::layout_constraintRight_toRightOf, ConstraintType::R2R},
    {Constants::layout_constraintRight_toLeftOf, ConstraintType::R2L},

//    {Constants::layout_constraintStart_toEndOf, ConstraintType::L2R},
//    {Constants::layout_constraintEnd_toStartOf, ConstraintType::R2L},
};

size_t Attribute::size() const {
  size_t res = 0;
  if (abs(value_primary) > 0.5) res++;

  if (IsCenterAnchor(type)) {
    res += 3;
    if (tgt_primary != tgt_secondary) res++;
    if (!tgt_primary->is_content_frame() || (tgt_secondary != nullptr && !tgt_secondary->is_content_frame())) res++;
  } else {
    res += 1;
    if (!tgt_primary->is_content_frame()) res++;
  }

  return res;
}

ModelWrapper::ModelWrapper() : ProbModel("FeatureModelDepreceated") {
  AddModel(std::unique_ptr<OrientationModel>(new OrientationModel()), 2.0);
  AddModel(std::unique_ptr<MarginModel>(new MarginModel()), 1.0);
  AddModel(std::unique_ptr<DistanceModel>(new DistanceModel()), 1.0);
  AddModel(std::unique_ptr<TypeModel>(new TypeModel()), 1.0);
  AddModel(std::unique_ptr<IntersectionModel>(new IntersectionModel()), 1.0);
  AddModel(std::unique_ptr<AttrSizeModel>(new AttrSizeModel()), 1.0);
}

LineSegment LineTo(const View* src, const View* tgt, ConstraintType type) {
  std::pair<int, int> points;
  switch(type) {
    case ConstraintType::T2B:
      points = ClosestPoint(src->xleft, src->xright, tgt->xleft, tgt->xright);
      return LineSegment(points.first, src->ytop, points.second, tgt->ybottom);
//      return LineSegment(src->xcenter(), src->ytop, tgt->xcenter(), tgt->ybottom);
    case ConstraintType::B2T:
      points = ClosestPoint(src->xleft, src->xright, tgt->xleft, tgt->xright);
      return LineSegment(points.first, src->ybottom, points.second, tgt->ytop);
//      return LineSegment(src->xcenter(), src->ybottom, tgt->xcenter(), tgt->ytop);
    case ConstraintType::R2L:
      points = ClosestPoint(src->ytop, src->ybottom, tgt->ytop, tgt->ybottom);
      return LineSegment(src->xright, points.first, tgt->xleft, points.second);
//      return LineSegment(src->xright, src->ycenter(), tgt->xleft, tgt->ycenter());
    case ConstraintType::L2R:
      points = ClosestPoint(src->ytop, src->ybottom, tgt->ytop, tgt->ybottom);
      return LineSegment(src->xleft, points.first, tgt->xright, points.second);
//      return LineSegment(src->xleft, src->ycenter(), tgt->xright, tgt->ycenter());
    case ConstraintType::L2L:
      points = ClosestPoint(src->ytop, src->ybottom, tgt->ytop, tgt->ybottom);
      return LineSegment(src->xleft, points.first, tgt->xleft, points.second);
//      return LineSegment(src->xleft, src->ycenter(), tgt->xleft, tgt->ycenter());
    case ConstraintType::R2R:
      points = ClosestPoint(src->ytop, src->ybottom, tgt->ytop, tgt->ybottom);
      return LineSegment(src->xright, points.first, tgt->xright, points.second);
//      return LineSegment(src->xright, src->ycenter(), tgt->xright, tgt->ycenter());
    case ConstraintType::T2T:
      points = ClosestPoint(src->xleft, src->xright, tgt->xleft, tgt->xright);
      return LineSegment(points.first, src->ytop, points.second, tgt->ytop);
//      return LineSegment(src->xcenter(), src->ytop, tgt->xcenter(), tgt->ytop);
    case ConstraintType::B2B:
      points = ClosestPoint(src->xleft, src->xright, tgt->xleft, tgt->xright);
      return LineSegment(points.first, src->ybottom, points.second, tgt->ybottom);
//      return LineSegment(src->xcenter(), src->ybottom, tgt->xcenter(), tgt->ybottom);
    default:
      LOG(FATAL) << "LineTo undefined for constraint type: " << static_cast<int>(type);
  }
}



bool IsRelationalAnchor(const ConstraintType& type) {
  return static_cast<int>(type) < static_cast<int>(ConstraintType::L2LxR2L);
}

bool IsCenterAnchor(const ConstraintType& type) {
  return !IsRelationalAnchor(type);
}

bool IsTypeStart(const ConstraintType& type) {
  switch(type) {
    case ConstraintType::L2L:
    case ConstraintType::L2R:
    case ConstraintType::T2T:
    case ConstraintType::T2B: return true;

    case ConstraintType::R2L:
    case ConstraintType::R2R:
    case ConstraintType::B2T:
    case ConstraintType::B2B: return false;
    default:
      LOG(FATAL) << "Unknown constraint type";
  }
  LOG(FATAL) << "Unknown constraint type";
}

std::pair<ConstraintType, ConstraintType> SplitAnchor(const ConstraintType& type) {
  switch(type) {
    case ConstraintType::L2L: return std::make_pair(ConstraintType::L2L, ConstraintType::LAST);
    case ConstraintType::L2R: return std::make_pair(ConstraintType::L2R, ConstraintType::LAST);
    case ConstraintType::R2L: return std::make_pair(ConstraintType::LAST, ConstraintType::R2L);
    case ConstraintType::R2R: return std::make_pair(ConstraintType::LAST, ConstraintType::R2R);
    case ConstraintType::T2T: return std::make_pair(ConstraintType::T2T, ConstraintType::LAST);
    case ConstraintType::T2B: return std::make_pair(ConstraintType::T2B, ConstraintType::LAST);
    case ConstraintType::B2T: return std::make_pair(ConstraintType::LAST, ConstraintType::B2T);
    case ConstraintType::B2B: return std::make_pair(ConstraintType::LAST, ConstraintType::B2B);

    case ConstraintType::L2LxR2L: return std::make_pair(ConstraintType::L2L, ConstraintType::R2L);
    case ConstraintType::L2LxR2R: return std::make_pair(ConstraintType::L2L, ConstraintType::R2R);
    case ConstraintType::L2RxR2L: return std::make_pair(ConstraintType::L2R, ConstraintType::R2L);
    case ConstraintType::L2RxR2R: return std::make_pair(ConstraintType::L2R, ConstraintType::R2R);
    case ConstraintType::T2TxB2T: return std::make_pair(ConstraintType::T2T, ConstraintType::B2T);
    case ConstraintType::T2TxB2B: return std::make_pair(ConstraintType::T2T, ConstraintType::B2B);
    case ConstraintType::T2BxB2T: return std::make_pair(ConstraintType::T2B, ConstraintType::B2T);
    case ConstraintType::T2BxB2B: return std::make_pair(ConstraintType::T2B, ConstraintType::B2B);
    default:
      LOG(FATAL) << "Unknown constraint type";
  }
  LOG(FATAL) << "Unknown constraint type";
};


std::pair<ConstraintType, ConstraintType> SplitCenterAnchor(const ConstraintType& type) {
  CHECK(IsCenterAnchor(type));
  switch(type) {
    case ConstraintType::L2LxR2L: return std::make_pair(ConstraintType::L2L, ConstraintType::R2L);
    case ConstraintType::L2LxR2R: return std::make_pair(ConstraintType::L2L, ConstraintType::R2R);
    case ConstraintType::L2RxR2L: return std::make_pair(ConstraintType::L2R, ConstraintType::R2L);
    case ConstraintType::L2RxR2R: return std::make_pair(ConstraintType::L2R, ConstraintType::R2R);
    case ConstraintType::T2TxB2T: return std::make_pair(ConstraintType::T2T, ConstraintType::B2T);
    case ConstraintType::T2TxB2B: return std::make_pair(ConstraintType::T2T, ConstraintType::B2B);
    case ConstraintType::T2BxB2T: return std::make_pair(ConstraintType::T2B, ConstraintType::B2T);
    case ConstraintType::T2BxB2B: return std::make_pair(ConstraintType::T2B, ConstraintType::B2B);
    default:
      LOG(FATAL) << "Trying to split non-center anchor!";
  }
  LOG(FATAL) << "Trying to split non-center anchor!";
};

ConstraintType GetCenterAnchor(const ConstraintType& start, const ConstraintType& end) {
  if (start == ConstraintType::L2L && end == ConstraintType::R2L) return ConstraintType ::L2LxR2L;
  if (start == ConstraintType::L2L && end == ConstraintType::R2R) return ConstraintType ::L2LxR2R;
  if (start == ConstraintType::L2R && end == ConstraintType::R2L) return ConstraintType ::L2RxR2L;
  if (start == ConstraintType::L2R && end == ConstraintType::R2R) return ConstraintType ::L2RxR2R;
  if (start == ConstraintType::T2T && end == ConstraintType::B2T) return ConstraintType ::T2TxB2T;
  if (start == ConstraintType::T2T && end == ConstraintType::B2B) return ConstraintType ::T2TxB2B;
  if (start == ConstraintType::T2B && end == ConstraintType::B2T) return ConstraintType ::T2BxB2T;
  if (start == ConstraintType::T2B && end == ConstraintType::B2B) return ConstraintType ::T2BxB2B;

  LOG(FATAL) << "Unknown constraint types!";
};

std::string ConstraintTypeToAttribute(const ConstraintType& cmd, const Constants::Type& output_type) {
  switch (cmd) {
    case ConstraintType::L2L: return Constants::name(Constants::layout_constraintLeft_toLeftOf, output_type);//"app:layout_constraintLeft_toLeftOf";
    case ConstraintType::L2R: return Constants::name(Constants::layout_constraintLeft_toRightOf, output_type);//"app:layout_constraintLeft_toRightOf";
    case ConstraintType::R2L: return Constants::name(Constants::layout_constraintRight_toLeftOf, output_type);//"app:layout_constraintRight_toLeftOf";
    case ConstraintType::R2R: return Constants::name(Constants::layout_constraintRight_toRightOf, output_type);//"app:layout_constraintRight_toRightOf";
    case ConstraintType::T2T: return Constants::name(Constants::layout_constraintTop_toTopOf, output_type);//"app:layout_constraintTop_toTopOf";
    case ConstraintType::T2B: return Constants::name(Constants::layout_constraintTop_toBottomOf, output_type);//"app:layout_constraintTop_toBottomOf";
    case ConstraintType::B2T: return Constants::name(Constants::layout_constraintBottom_toTopOf, output_type);//"app:layout_constraintBottom_toTopOf";
    case ConstraintType::B2B: return Constants::name(Constants::layout_constraintBottom_toBottomOf, output_type);//"app:layout_constraintBottom_toBottomOf";
    default:
      LOG(FATAL) << "Unknown constraint type";
  }
  LOG(FATAL) << "Unknown constraint type";
}

std::string ConstraintTypeToMargin(const ConstraintType& cmd, const Constants::Type& output_type) {
  switch (cmd) {
    case ConstraintType::L2L: return Constants::name(Constants::layout_marginLeft, output_type);//"android:layout_marginLeft";
    case ConstraintType::L2R: return Constants::name(Constants::layout_marginLeft, output_type);//"android:layout_marginLeft";
    case ConstraintType::R2L: return Constants::name(Constants::layout_marginRight, output_type);//"android:layout_marginRight";
    case ConstraintType::R2R: return Constants::name(Constants::layout_marginRight, output_type);//"android:layout_marginRight";
    case ConstraintType::T2T: return Constants::name(Constants::layout_marginTop, output_type);//"android:layout_marginTop";
    case ConstraintType::T2B: return Constants::name(Constants::layout_marginTop, output_type);//"android:layout_marginTop";
    case ConstraintType::B2T: return Constants::name(Constants::layout_marginBottom, output_type);//"android:layout_marginBottom";
    case ConstraintType::B2B: return Constants::name(Constants::layout_marginBottom, output_type);//"android:layout_marginBottom";
    default:
      LOG(FATAL) << "Unknown constraint type";
  }
  LOG(FATAL) << "Unknown constraint type";
}

Constants::Name ConstraintTypeToMargin(const Constants::Name& cmd) {
  switch (cmd) {
    case Constants::layout_constraintLeft_toLeftOf: return Constants::layout_marginLeft;
    case Constants::layout_constraintLeft_toRightOf: return Constants::layout_marginLeft;
    case Constants::layout_constraintRight_toLeftOf: return Constants::layout_marginRight;
    case Constants::layout_constraintRight_toRightOf: return Constants::layout_marginRight;
    case Constants::layout_constraintTop_toTopOf: return Constants::layout_marginTop;
    case Constants::layout_constraintTop_toBottomOf: return Constants::layout_marginTop;
    case Constants::layout_constraintBottom_toTopOf: return Constants::layout_marginBottom;
    case Constants::layout_constraintBottom_toBottomOf: return Constants::layout_marginBottom;
    case Constants::NO_NAME: return Constants::NO_NAME;
    default:
      LOG(FATAL) << "Unknown constraint type";
  }
  LOG(FATAL) << "Unknown constraint type";
}

std::string ConstraintTypeStr(const ConstraintType& cmd) {
  switch (cmd) {
    case ConstraintType::L2L: return "L2L";
    case ConstraintType::L2R: return "L2R";
    case ConstraintType::R2L: return "R2L";
    case ConstraintType::R2R: return "R2R";
    case ConstraintType::T2T: return "T2T";
    case ConstraintType::T2B: return "T2B";
    case ConstraintType::B2T: return "B2T";
    case ConstraintType::B2B: return "B2B";
    case ConstraintType::L2LxR2L: return "L2LxR2L";
    case ConstraintType::L2LxR2R: return "L2LxR2R";
    case ConstraintType::L2RxR2L: return "L2RxR2L";
    case ConstraintType::L2RxR2R: return "L2RxR2R";
    case ConstraintType::T2TxB2T: return "T2TxB2T";
    case ConstraintType::T2TxB2B: return "T2TxB2B";
    case ConstraintType::T2BxB2T: return "T2BxB2T";
    case ConstraintType::T2BxB2B: return "T2BxB2B";
    default:
      LOG(FATAL) << "Unknown constraint type";
  }
  LOG(FATAL) << "Unknown constraint type";
}

ConstraintType StrToConstraintType(const std::string& value) {
  for (int i = 0; i < static_cast<int>(ConstraintType::LAST); i++) {
    if (ConstraintTypeStr(static_cast<ConstraintType>(i)) == value) {
      return static_cast<ConstraintType>(i);
    }
  }
  LOG(FATAL) << "Unknown constraint type '" << value << "'";
}

Orientation ConstraintTypeToOrientation(const ConstraintType& cmd) {
  switch (cmd) {
    case ConstraintType::L2L:
    case ConstraintType::L2R:
    case ConstraintType::R2L:
    case ConstraintType::R2R:
    case ConstraintType::L2LxR2L:
    case ConstraintType::L2LxR2R:
    case ConstraintType::L2RxR2L:
    case ConstraintType::L2RxR2R:
      return Orientation::HORIZONTAL;
    case ConstraintType::T2T:
    case ConstraintType::T2B:
    case ConstraintType::B2T:
    case ConstraintType::B2B:
    case ConstraintType::T2TxB2T:
    case ConstraintType::T2TxB2B:
    case ConstraintType::T2BxB2T:
    case ConstraintType::T2BxB2B:
      return Orientation::VERTICAL;
  };
  LOG(FATAL) << "Unknown constraint type";
}

std::unordered_map<std::string, const View> GetNodesWithId(const ProtoScreen& app) {
  std::unordered_map<std::string, const View> nodes;
  for (const ProtoView& view : app.views()) {
    if (Contains(view.properties(), Constants::name(Constants::ID))) {
      nodes.emplace(view.properties().at(Constants::name(Constants::ID)), View(&view));
    }
  }
  return nodes;
};

void ForEachConstraintLayoutRelation(const ProtoScreen& app, const std::function<void(const Constants::Name&, const View&, const View&, const std::vector<View>&)>& cb) {
  const std::unordered_map<std::string, const View>& id_to_nodes = GetNodesWithId(app);
  std::vector<View> views;
  for (const ProtoView& view : app.views()) {
    views.emplace_back(View(&view));
  }
//  LOG(INFO) << "Num views with id: " << id_to_nodes.size();
  for(const auto& it : id_to_nodes) {
//    LOG(INFO) << it.second.id;
    for (const Constants::Name &property_name : Properties::CONSTRAINT_LAYOUT_CONSTRAINTS) {
      const std::string& property = Constants::name(property_name);
      if (Contains(it.second.proto_view->properties(), property)) {
        std::string id = it.second.proto_view->properties().at(property);
        if (id != "parent" and !Contains(id_to_nodes, id)) {
          continue;
        }
        const View &ref_view = (id != "parent") ? id_to_nodes.at(id) : views[it.second.proto_view->parent_seq_id()];
        cb(property_name, it.second, ref_view, views);
      }
    }
  }
}

void ForEachRelativeLayoutRelation(const ProtoScreen& app, const std::function<void(const Constants::Name&, const View&, const View&, const std::vector<View>&)>& cb) {
  const std::unordered_map<std::string, const View>& id_to_nodes = GetNodesWithId(app);
  std::vector<View> views;
  for (const ProtoView& view : app.views()) {
    views.emplace_back(View(&view));
  }

  for(const auto& it : id_to_nodes) {
    for (const Constants::Name &property_name : Properties::RELATIVE_CONSTRAINTS) {
      const std::string& property = Constants::name(property_name);
      if (Contains(it.second.proto_view->properties(), property)) {
        std::string id = it.second.proto_view->properties().at(property);
        if (!Contains(id_to_nodes, id)) {
          continue;
        }
        const View &ref_view = id_to_nodes.at(id);
        cb(property_name, it.second, ref_view, views);
      }
    }
  }
}


bool View::IsAnchored(const Orientation& orientation, std::unordered_set<int>& visited) const {
  if (is_content_frame()) return true;

  if (!Contains(attributes, orientation)) {
    return false;
  }

  visited.insert(id);
  const Attribute& attr = attributes.at(orientation);
  if (IsRelationalAnchor(attr.type)) {
    if (Contains(visited, attr.tgt_primary->id)) return false;

    return attr.tgt_primary->IsAnchored(orientation, visited);
  } else {
    CHECK_EQ(IsCenterAnchor(attr.type), true);
    if (Contains(visited, attr.tgt_primary->id)) return false;
    if (Contains(visited, attr.tgt_secondary->id)) return false;

    // Necessary in order to enable one child to depend on the other
    std::unordered_set<int> inner_visited;
    inner_visited.insert(id);
    return attr.tgt_primary->IsAnchored(orientation, visited) && attr.tgt_secondary->IsAnchored(orientation, inner_visited);
  }
}

void View::ReferencedNodesInner(const Orientation& orientation, std::unordered_set<int>& visited) const {
  visited.insert(id);

  if (!Contains(attributes, orientation)) {
    return;
  }

  const Attribute& attr = attributes.at(orientation);
  if (IsRelationalAnchor(attr.type)) {
    if (!Contains(visited, attr.tgt_primary->id)) {
      attr.tgt_primary->ReferencedNodesInner(orientation, visited);
    }
  } else {
    CHECK_EQ(IsCenterAnchor(attr.type), true);
    if (!Contains(visited, attr.tgt_primary->id)) {
      attr.tgt_primary->ReferencedNodesInner(orientation, visited);
    }
    if (!Contains(visited, attr.tgt_secondary->id)) {
      attr.tgt_secondary->ReferencedNodesInner(orientation, visited);
    }
  }
}

bool View::IsCircularRelation(const Orientation& orientation, const Attribute& attr) const {
  if (IsRelationalAnchor(attr.type)) {
    return Contains(attr.tgt_primary->ReferencedNodes(orientation), id);
  } else {
    CHECK_EQ(IsCenterAnchor(attr.type), true);
    return Contains(attr.tgt_primary->ReferencedNodes(orientation), id) || Contains(attr.tgt_secondary->ReferencedNodes(orientation), id);
  }
}

bool View::IsAnchoredWithAttr(const Orientation& orientation, const Attribute& attr) const {
  if (IsRelationalAnchor(attr.type)) {
    return attr.tgt_primary->IsAnchored(orientation);
  } else {
    CHECK_EQ(IsCenterAnchor(attr.type), true);
    return attr.tgt_primary->IsAnchored(orientation) && attr.tgt_secondary->IsAnchored(orientation);
  }
}

Json::Value Attribute::ToJSON(const std::vector<int> seq_to_pos, const std::vector<int> swaps) const {
	Json::Value attrJson(Json::objectValue);
	attrJson["type"] =  ConstraintTypeStr(type);
	attrJson["prob"] =  prob;
	attrJson["size"] =  ViewSizeStr(view_size);
	attrJson["val_primary"] =  value_primary;
	attrJson["val_secondary"] = value_secondary;
	attrJson["bias"] =  bias;
	attrJson["tgt_prim"] =  -1;
	attrJson["tgt_scnd"] =  -1;

	if(swaps.size() == 0){
		attrJson["srcid"] = seq_to_pos[src->id];
		if(tgt_primary != nullptr){
			attrJson["tgt_prim"] = seq_to_pos[tgt_primary->id];
		}
		if(tgt_secondary != nullptr){
			attrJson["tgt_scnd"] = seq_to_pos[tgt_secondary->id];
		}
	}else{
		attrJson["srcid"] =  swaps[seq_to_pos[src->id]];
		if(tgt_primary != nullptr){
			attrJson["tgt_prim"] = swaps[seq_to_pos[tgt_primary->id]];
		}
		if(tgt_secondary != nullptr){
			attrJson["tgt_scnd"] = swaps[seq_to_pos[tgt_secondary->id]];
		}

	}
	return attrJson;
}


void Attribute::CenterToProperties(std::unordered_map<std::string, std::string>& properties, const Constants::Type& output_type) const {
  auto types = SplitCenterAnchor(type);
  Attribute(types.first, view_size, value_primary, src, tgt_primary).ToProperties(properties, output_type);
  Attribute(types.second, view_size, value_secondary, src, tgt_secondary).ToProperties(properties, output_type);

  if (bias != 0.5) {
    if (ConstraintTypeToOrientation(type) == Orientation::HORIZONTAL) {
      properties[Constants::name(Constants::layout_constraintHorizontal_bias, output_type)] = std::to_string(bias);
    } else {
      properties[Constants::name(Constants::layout_constraintVertical_bias, output_type)] = std::to_string(bias);
    }

  }
}

void Attribute::AlignToProperties(std::unordered_map<std::string, std::string>& properties, const Constants::Type& output_type) const {
  if (tgt_primary->id == 0) {
    properties[ConstraintTypeToAttribute(type, output_type)] = "parent";
  } else {
    properties[ConstraintTypeToAttribute(type, output_type)] = tgt_primary->id_string;//StringPrintf("@+id/view%d", tgt_primary->id);
  }


  CHECK(value_primary == 0 || value_secondary == 0);
  if (value_primary != 0) {
    properties[ConstraintTypeToMargin(type, output_type)] = StringPrintf("%dpx", value_primary);
  }
  if (value_secondary != 0) {
    properties[ConstraintTypeToMargin(type, output_type)] = StringPrintf("%dpx", value_secondary);
  }
}

Attribute::Attribute(
    View* src,
    ViewSize view_size,
    int value_primary, int value_secondary,
    const View* start_view, const Constants::Name& start_type,
    const View* end_view, const Constants::Name& end_type, float bias) : view_size(view_size), value_primary(value_primary), value_secondary(value_secondary), src(src), prob(0), bias(bias) {

  if (start_type == Constants::NO_NAME) {
    type = AttrModel::ATTRIBUTE_TO_TYPE.at(end_type);
    tgt_primary = end_view;
    tgt_secondary = nullptr;
  } else if (end_type == Constants::NO_NAME) {
    type = AttrModel::ATTRIBUTE_TO_TYPE.at(start_type);
    tgt_primary = start_view;
    tgt_secondary = nullptr;
  } else {
    type = GetCenterAnchor(AttrModel::ATTRIBUTE_TO_TYPE.at(start_type), AttrModel::ATTRIBUTE_TO_TYPE.at(end_type));
    tgt_primary = start_view;
    tgt_secondary = end_view;
  }
}

void PrintApp(std::stringstream& ss, const App& app, bool with_attributes) {
  ss << "App:\n";
  for (const View& view : app.GetViews()) {
    ss << view << '\n';
    if (view.id == 0) continue;
    if (with_attributes) {
      ss << '\t' << view.attributes.at(Orientation::HORIZONTAL) << '\n';
      ss << '\t' << view.attributes.at(Orientation::VERTICAL) << '\n';
    }
  }
}

void PrintApp(const App& app, bool with_attributes) {
  LOG(INFO) << "App:";
  for (const View& view : app.GetViews()) {
    LOG(INFO) << view;
    if (view.id == 0) continue;
    if (with_attributes) {
      LOG(INFO) << '\t' << view.attributes.at(Orientation::HORIZONTAL);
      LOG(INFO) << '\t' << view.attributes.at(Orientation::VERTICAL);
    }
  }
}
void WriteAppToFile(const App& app, std::string name, bool with_attributes) {
     std::ofstream out(name);
  for (const View& view : app.GetViews()) {
    out << view << std::endl;
    if (view.id == 0) continue;
      if (with_attributes) {
          out << '\t' << view.attributes.at(Orientation::HORIZONTAL) << "\n";
          out << '\t' << view.attributes.at(Orientation::VERTICAL) << "\n";
     }
  }
  out.close();
}
