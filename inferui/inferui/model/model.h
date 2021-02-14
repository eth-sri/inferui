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

#ifndef CC_SYNTHESIS_MODEL_H
#define CC_SYNTHESIS_MODEL_H

#include <string>
#include <cmath>
#include <unordered_map>



#include <glog/logging.h>

#include "base/stringprintf.h"
#include "base/maputil.h"
#include "base/geomutil.h"
#include "base/strutil.h"
#include "base/counter.h"
#include "base/base.h"

#include "inferui/model/util/constants.h"
#include "inferui/model/uidump.pb.h"
#include "json/json.h"
#include "inferui/model/util/util.h"


enum class ConstraintType {
  L2L = 0,
  L2R,
  R2L,
  R2R,

  T2T,
  T2B,
  B2T,
  B2B,

  L2LxR2L,
  L2LxR2R,
  L2RxR2L,
  L2RxR2R,

  T2TxB2T,
  T2TxB2B,
  T2BxB2T,
  T2BxB2B,

  LAST
};

namespace std {
  template <> struct hash<ConstraintType> {
    size_t operator()(const ConstraintType& x) const {
      return static_cast<int>(x);
    }
  };
}




std::string ConstraintTypeStr(const ConstraintType& cmd);

ConstraintType StrToConstraintType(const std::string& value);

Orientation ConstraintTypeToOrientation(const ConstraintType& cmd);

// For serializing results
std::string ConstraintTypeToAttribute(const ConstraintType& cmd, const Constants::Type& output_type);
std::string ConstraintTypeToMargin(const ConstraintType& cmd, const Constants::Type& output_type);
Constants::Name ConstraintTypeToMargin(const Constants::Name& cmd);

bool IsRelationalAnchor(const ConstraintType& type);

bool IsCenterAnchor(const ConstraintType& type);

bool IsTypeStart(const ConstraintType& type);

std::pair<ConstraintType, ConstraintType> SplitAnchor(const ConstraintType& type);
std::pair<ConstraintType, ConstraintType> SplitCenterAnchor(const ConstraintType& type);
ConstraintType GetCenterAnchor(const ConstraintType& start, const ConstraintType& end);

class View;

class Attribute {
public:
  Attribute(ConstraintType type, ViewSize view_size, int value, View* src, const View* primary) : Attribute(type, view_size, IsTypeStart(type) ? value : 0, !IsTypeStart(type) ? value : 0, src, primary, nullptr) {
  }

  Attribute(ConstraintType type, ViewSize view_size, int value_primary, int value_secondary, View* src, const View* primary) : Attribute(type, view_size, value_primary, value_secondary, src, primary, nullptr) {
  }

  Attribute(ConstraintType type, ViewSize view_size, int value_primary, View* src, const View* primary, const View* secondary) : Attribute(type, view_size, value_primary, 0, src, primary, secondary) {

  }

  Attribute(ConstraintType type, ViewSize view_size, int value_primary, int value_secondary, View* src, const View* primary, const View* secondary, float bias = 0.5) :
      type(type), view_size(view_size), value_primary(value_primary), value_secondary(value_secondary), src(src), tgt_primary(primary), tgt_secondary(secondary), prob(0), bias(bias) {
    CHECK(!IsCenterAnchor(type) || secondary != nullptr);
    CHECK(!IsRelationalAnchor(type) || secondary == nullptr);

    CHECK_GE(value_primary, 0);
    CHECK_GE(value_secondary, 0);
  }

  Attribute(View* src, ViewSize view_size, int value_primary, int value_secondary, const View* start_view, const Constants::Name& start_type, const View* end_view, const Constants::Name& end, float bias);

  size_t size() const;

  bool operator==(const Attribute& other) const {
    return type == other.type && view_size == other.view_size &&
        value_primary == other.value_primary && value_secondary == other.value_secondary && bias == other.bias &&
           EqualOrNull(src, other.src) &&
           EqualOrNull(tgt_primary, other.tgt_primary) &&
           EqualOrNull(tgt_secondary, other.tgt_secondary);
  }

  bool operator<(const Attribute &other) const {
    return prob < other.prob;
  }

  bool operator>(const Attribute &other) const {
    return prob > other.prob;
  }

  friend std::ostream& operator<<(std::ostream& os, const Attribute& attr);

  void ToProperties(std::unordered_map<std::string, std::string>& properties, const Constants::Type& output_type) const {
    if (IsCenterAnchor(type)){//tgt_secondary != nullptr) {
      CenterToProperties(properties, output_type);
    } else {
      AlignToProperties(properties, output_type);
    }
  }

  void CenterToProperties(std::unordered_map<std::string, std::string>& properties, const Constants::Type& output_type) const;
  void AlignToProperties(std::unordered_map<std::string, std::string>& properties, const Constants::Type& output_type) const;


  Json::Value ToJSON(const std::vector<int> seq_to_pos, const std::vector<int> swaps = std::vector<int>()) const;



  ConstraintType type;
  ViewSize view_size;

  int value_primary;
  int value_secondary;
//  double value

  View* src;
  const View* tgt_primary;
  const View* tgt_secondary;

  double prob;

  float bias;
};



LineSegment LineTo(const View* src, const View* tgt, ConstraintType type);

class View {
public:
  View(int left, int top, int right, int bottom, std::string name, int id, const ProtoView* view = nullptr) :
      xleft(left), xright(right), ytop(top), ybottom(bottom), name(name), id(id), pos(-1), id_string(id == 0 ? "parent" : StringPrintf("@+id/view%d", id)), proto_view(view) {
  }

  View(int left, int top, int right, int bottom, std::string name, int id, std::string id_string) :
      xleft(left), xright(right), ytop(top), ybottom(bottom), name(name), id(id), pos(-1), id_string(id_string), proto_view(nullptr) {
  }

  View(const ProtoView* view)
      : View(view->xleft(), view->ytop(), view->xright(), view->ybottom(), view->type(), view->seq_id(), view) {
  }

  View(const ProtoView& view) : View(view.xleft(), view.ytop(), view.xright(), view.ybottom(), view.type(), view.seq_id(), nullptr) {
//    LOG(INFO) << "\t" << ViewSizeStr(GetViewSize(view, Orientation::HORIZONTAL));
//    LOG(INFO) << "\t" << ViewSizeStr(GetViewSize(view, Orientation::VERTICAL));
  }

  bool is_content_frame() const {
    return id == 0;
  }

  int width() const {
    return xright - xleft;
  }

  int height() const {
    return ybottom - ytop;
  }

  double xcenter() const {
    return (xright + xleft) / 2.0;
  }

  double ycenter() const {
    return (ytop + ybottom) / 2.0;
  }

  std::unordered_set<int> ReferencedNodes(const Orientation& orientation) const {
    std::unordered_set<int> visited;
    ReferencedNodesInner(orientation, visited);
    return visited;
  }

  bool IsCircularRelation(const Orientation& orientation, const Attribute& attr) const;

  bool IsAnchoredWithAttr(const Orientation& orientation, const Attribute& attr) const;

  bool IsAnchored(const Orientation& orientation) const {
    std::unordered_set<int> visited;
    return IsAnchored(orientation, visited);
  }

  bool HasAttribute(const Orientation& orientation) const {
    return Contains(attributes, orientation);
  }

  const Attribute& GetAttribute(const Orientation& orientation) const {
    CHECK(HasAttribute(orientation));
    return attributes.find(orientation)->second;
  }

  double GetAttributeProb(const Orientation& orientation) const {
    CHECK(Contains(attributes, orientation));
    return attributes.at(orientation).prob;
  }

  void ApplyAttribute(const Orientation& orientation, const Attribute& attr) {
    CHECK_EQ(this, attr.src);
    auto it = attributes.find(orientation);
    if (it == attributes.end()) {
      attributes.insert({orientation, attr});
    } else {
      it->second = attr;
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const View& node);

  bool operator==(const View& other) const {
    return id == other.id;
  }

  bool HasFixedPosition() const {
    return xleft != -1 && xright != -1 && ytop != -1 && ybottom != -1;
  }

  void setPosition(const View& other) {
     xleft = other.xleft;
     xright = other.xright;
     ytop = other.ytop;
     ybottom = other.ybottom;
  }

  int xleft;
  int xright;
  int ytop;
  int ybottom;

  Padding padding;

  std::string name;
  // sequential id within the list of all components
  int id;
  // sequential id within the list of synthesized components (subset of all components) used by App
  int pos;
  std::string id_string;

  const ProtoView* proto_view;

  std::unordered_map<Orientation, Attribute> attributes;

  std::unordered_map<Orientation, ViewSize> view_size;

  Json::Value ToJSON(const Constants::Type& output_type) const {
    Json::Value res(Json::objectValue);
    for (auto it : ToProperties(output_type)) {
      res[it.first] = it.second;
    }
    return res;
  }

  Json::Value ToCoordinatesJSON(const Constants::Type& output_type) const {
    Json::Value res(Json::objectValue);
    res["id"] = id;
    res["x"] = xleft;
    res["y"] = ytop;
    res["width"] = width();
    res["height"] = height();
    return res;
  }

  std::unordered_map<std::string, std::string> ToProperties(const Constants::Type& output_type) const {
    std::unordered_map<std::string, std::string> properties;
    properties[Constants::name(Constants::ID, output_type)] = id_string;//StringPrintf("@+id/view%d", id);
    padding.ToProperties(output_type, properties);

    if (!is_content_frame()) {
      CHECK_EQ(attributes.size(), 2);
      for (const auto& it : attributes) {
        if (it.first == Orientation::HORIZONTAL) {
          properties[Constants::name(Constants::layout_width, output_type)] = ViewSizeStr(it.second.view_size, (xright - xleft));
        } else if (it.first == Orientation::VERTICAL) {
          properties[Constants::name(Constants::layout_height, output_type)] = ViewSizeStr(it.second.view_size, (ybottom - ytop));
        }
      }
    } else {
//      CHECK((xright - xleft) % 2 == 0) << *this;
//      CHECK((ybottom - ytop) % 2 == 0) << *this;
      properties[Constants::name(Constants::layout_width, output_type)] = StringPrintf("%dpx", (xright - xleft));
      properties[Constants::name(Constants::layout_height, output_type)] = StringPrintf("%dpx", (ybottom - ytop));
    }



    for (const auto& it : attributes) {
      it.second.ToProperties(properties, output_type);
    }

    return properties;
  }

  std::string ToXml(bool properties_only=false) const {
    std::unordered_map<std::string, std::string> properties = ToProperties(Constants::Type::OUTPUT_XML);

    std::stringstream ss;

    if (!properties_only) {
      ss << "<" << name << "\n";
    }
    for (const auto& it : properties) {
      ss << "\t" << it.first << "=\"" << it.second << "\"\n";
    }
    if (!properties_only) {
      ss << "/>";
    }

    return ss.str();
  }

private:
  bool IsAnchored(const Orientation& orientation, std::unordered_set<int>& visited) const;

  void ReferencedNodesInner(const Orientation& orientation, std::unordered_set<int>& visited) const;
};

class App {
public:

  App & operator=(const App& other) {
    seq_id_to_pos = other.seq_id_to_pos;
    resizable = other.resizable;

    views.clear();
    for (const View& view : other.views) {
      AddView(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
    }
    copyAttributes(other);
    return *this;
  }


  App() {

  }


  App(const Json::Value json) {
	  int i = 0;
	  for(auto view: json["Views"]){
		  int x = view["x"].asInt();
		  int y = view["y"].asInt();
	      AddView(View(x, y, x + view["width"].asInt(), y + view["height"].asInt(), "-1", i));
	      i++;
	  }
  }


  App(const ProtoScreen& app, bool only_constraint_views = false) : seq_id_to_pos(app.views_size(), 0) {
    for (const ProtoView& view : app.views()) {
      if (!only_constraint_views || InRootConstraintLayout(app, view)) {
        seq_id_to_pos[view.seq_id()] = views.size();
        AddView(View(view));
      }
    }
//    LOG(INFO) << "Number of views: " << views.size();
    CHECK(views[0].name == "android.support.constraint.ConstraintLayout") << views[0].name;

    InitializeResizable(app);
  }


  App(const App& other){
	  	//maybe call assigment operator here instead
	  	//*this = app;
	    seq_id_to_pos = other.seq_id_to_pos;
	    resizable = other.resizable;

	    views.clear();
	    for (const View& view : other.views) {
	       AddView(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
	    }
	    copyAttributes(other);
  }

  App(const std::vector<View>& otherViews, int numberOfViews, std::vector<bool> otherResizable){
	    //seq_id_to_pos = other.seq_id_to_pos; only needed in copyAttributes -> we do not do that anyways here... watch out if this is used in a different context
	    resizable = otherResizable;

	    views.clear();
	    for (const View& view : otherViews) {
	      AddView(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
	      if((int) views.size() == numberOfViews){
	    	  break;
	      }
	    }
  }

  //merge a vertical and horizontal view together
  App (const App& vertical, const App& horizontal) {
	//TODO (larissa) is this ok, to take it from only one of them?
    seq_id_to_pos = vertical.seq_id_to_pos;
    resizable = vertical.resizable;

    views.clear();
	//watch out seq_id_to_pos size might be not true (only in my reordered example, in which view id is reset)
	seq_id_to_pos = std::vector<int>(vertical.views.size(), 0);


    for (const View& view : vertical.views) {
    	//TODO (larissa) make sure that this can never get out of the synthesizer
    	if(view.ytop == -1 || view.ybottom == -1){
    		LOG(INFO) << "Unexpected view coordinate input" << view.ytop << " " << view.ybottom;
    	}
    	const View& corresponsing = horizontal.findView(view.id);
    	if(corresponsing.xleft == -1 || corresponsing.xright == -1){
    		LOG(INFO) << "Unexpected view coordinate input" << corresponsing.xleft << " " << corresponsing.xright;
    	}
    	if(view.id >= static_cast<int>(seq_id_to_pos.size())){
    		LOG(FATAL) << "This should not happen.  " << view.id << " " << seq_id_to_pos.size();
    	}
    	seq_id_to_pos[view.id] = views.size();
    	AddView(View(corresponsing.xleft, view.ytop, corresponsing.xright, view.ybottom, view.name, view.id));
    }
    copyAttributes(vertical);
    copyAttributes(horizontal);
  }


  void setResizable(std::vector<bool> _resizable){
    resizable.clear();
    resizable = _resizable;
  }

  void copyAttributes(const App& other){
    for (size_t id = 0; id < other.views.size(); id++) {
      View& view = views[id];
      const View& other_view = other.views[id];
      for (const auto& it : other_view.attributes) {

        Attribute attr(
            it.second.type,
            it.second.view_size,
            it.second.value_primary,
            it.second.value_secondary,
            &findView(it.second.src->id),
            &findView(it.second.tgt_primary->id),
            (it.second.tgt_secondary != nullptr) ? &findView(it.second.tgt_secondary->id) : nullptr,
            it.second.bias
        );
        //TODO (larissa) double check: is this a problem in other cases
        attr.prob = it.second.prob;
        view.attributes.insert({it.first, std::move(attr)});
      }
    }
  }

  void InitializeResizable(const ProtoScreen& screen) {
    resizable.clear();
    resizable.push_back(GetViewSize(screen.views(0), Orientation::HORIZONTAL) != ViewSize::FIXED);
    resizable.push_back(GetViewSize(screen.views(0), Orientation::VERTICAL) != ViewSize::FIXED);
//    if (!resizable[0] || !resizable[1]) {
//      LOG(INFO) << screen.views(0).DebugString();
//      LOG(INFO) << '\t' << resizable[0] << " " << resizable[1];
//    }
  }

  void SetResizable(const Device& ref, const std::vector<Device>& all) {
    resizable.clear();
    resizable.push_back(false);
    resizable.push_back(false);
    for (const Device& dev : all) {
      if (ref.width != dev.width) {
        resizable[0] = true;
      }
      if (ref.height != dev.height) {
        resizable[1] = true;
      }
    }
  }

  bool IsResizable(const Orientation& orientation) const {
    CHECK_EQ(resizable.size(), 2);
    return resizable[static_cast<int>(orientation)];
  }

  void InitializeAttributes(const ProtoScreen& app) {
    InitializeResizable(app);
    for (View& view : views) {
      if (view.is_content_frame()) {
        view.padding.Initialize(app.views(view.id));
        continue;
      }

      const ProtoView& ref_view = app.views(view.id);
      {
        std::pair<const ProtoView *, Constants::Name> start = FindPropertyTarget(app, ref_view,
                                                                                Properties::LEFT_CONSTRAINT_LAYOUT_CONSTRAINTS);
        std::pair<const ProtoView *, Constants::Name> end = FindPropertyTarget(app, ref_view,
                                                                                 Properties::RIGHT_CONSTRAINT_LAYOUT_CONSTRAINTS);
        CHECK(start.first != nullptr || end.first != nullptr);

        Attribute attr(&view,
                       GetViewSize(ref_view, Orientation::HORIZONTAL),
                       GetMarginFromProto(ref_view, ConstraintTypeToMargin(start.second)),
                       GetMarginFromProto(ref_view, ConstraintTypeToMargin(end.second)),
                       (start.first != nullptr) ? &views[seq_id_to_pos[start.first->seq_id()]] : nullptr, start.second,
                       (end.first != nullptr) ? &views[seq_id_to_pos[end.first->seq_id()]] : nullptr, end.second,
                       GetBiasFromProto(ref_view, Orientation::HORIZONTAL));

        if (attr.view_size == ViewSize::MATCH_PARENT &&
            attr.tgt_primary->id == 0 && attr.tgt_secondary != nullptr && attr.tgt_secondary->id == 0) {
          attr.view_size = ViewSize::MATCH_CONSTRAINT;
        }
        if (attr.view_size == ViewSize::MATCH_CONSTRAINT && IsRelationalAnchor(attr.type)) {
          attr.view_size = ViewSize::FIXED;
        }
        view.attributes.insert({Orientation::HORIZONTAL, attr});
      }

      {
        std::pair<const ProtoView *, Constants::Name> start = FindPropertyTarget(app, ref_view,
                                                                               Properties::TOP_CONSTRAINT_LAYOUT_CONSTRAINTS);
        std::pair<const ProtoView *, Constants::Name> end = FindPropertyTarget(app, ref_view,
                                                                                  Properties::BOTTOM_CONSTRAINT_LAYOUT_CONSTRAINTS);
        CHECK(start.first != nullptr || end.first != nullptr);

        Attribute attr(&view,
                       GetViewSize(ref_view, Orientation::VERTICAL),
                       GetMarginFromProto(ref_view, ConstraintTypeToMargin(start.second)),
                       GetMarginFromProto(ref_view, ConstraintTypeToMargin(end.second)),
                       (start.first != nullptr) ? &views[seq_id_to_pos[start.first->seq_id()]] : nullptr, start.second,
                       (end.first != nullptr) ? &views[seq_id_to_pos[end.first->seq_id()]] : nullptr, end.second,
                       GetBiasFromProto(ref_view, Orientation::VERTICAL));
        if (attr.view_size == ViewSize::MATCH_PARENT &&
            attr.tgt_primary->id == 0 && attr.tgt_secondary != nullptr && attr.tgt_secondary->id == 0) {
          attr.view_size = ViewSize::MATCH_CONSTRAINT;
        }
        if (attr.view_size == ViewSize::MATCH_CONSTRAINT && IsRelationalAnchor(attr.type)) {
          attr.view_size = ViewSize::FIXED;
        }
        view.attributes.insert({Orientation::VERTICAL, attr});
      }

    }
  }

  View& findView(int id) {
    for (size_t i = 0; i < views.size(); i++) {
      if (views[i].id == id) {
        return views[i];
      }
    }
    LOG(FATAL) << "View with id " << id << " not found!";
  }

  const View& findView(int id) const {
    for (size_t i = 0; i < views.size(); i++) {
      if (views[i].id == id) {
        return views[i];
      }
    }
    LOG(FATAL) << "View with id " << id << " not found!";
  }

  Json::Value ToJSON(const Constants::Type& output_type = Constants::Type::OUTPUT_XML) const {
    Json::Value res(Json::objectValue);
    {
      Json::Value layout(Json::arrayValue);
      for (const auto &view : views) {
        layout.append(view.ToJSON(output_type));
      }
      layout[0][Constants::name(Constants::ID, output_type)] = "parent";
      res["layout"] = layout;
    }

    res["x_offset"] = views[0].xleft;
    res["y_offset"] = views[0].ytop;

    return res;
  }

  Json::Value ToCoordinatesJSON(const Constants::Type& output_type = Constants::Type::OUTPUT_XML) const {
    Json::Value res(Json::objectValue);
    {
      Json::Value layout(Json::arrayValue);
      for (unsigned i = 0; i < views.size(); i++) {
        layout.append(views[i].ToCoordinatesJSON(output_type));
      }
      res["Views"] = layout;
      res["layout_id"] = -1;
    }
    return res;
  }

  Json::Value ToCoordinatesJSON(int layout_id, const App& candidate, Json::Value constraints, std::pair<double, double> layout_prob, std::vector<std::pair<double, double>> layout_prob_individual, const Constants::Type& output_type = Constants::Type::OUTPUT_XML) const {
	 //TOCHECK
	  for(auto& view : candidate.GetViews()){
		 if(view.id != candidate.seq_id_to_pos[view.id]){
			 LOG(FATAL) << "in reordered cases pos should be = id";
		 }
	 }

	  Json::Value res(Json::objectValue);
    {
      Json::Value layout(Json::arrayValue);
      for (unsigned i = 0; i < views.size(); i++) {
    	auto view_json = views[i].ToCoordinatesJSON(output_type);
    		view_json["prob_hori"] = layout_prob_individual[i].first;
    		view_json["prob_vert"] = layout_prob_individual[i].second;
    		if(i != 0){
    			view_json["vert_const"] = candidate.GetViews()[i].attributes.at(Orientation::VERTICAL).ToJSON(candidate.seq_id_to_pos);
    			view_json["hort_const"] = candidate.GetViews()[i].attributes.at(Orientation::HORIZONTAL).ToJSON(candidate.seq_id_to_pos);
    		}
    		layout.append(view_json);
      }
      res["Views"] = layout;
      res["layout_id"] = layout_id;
      res["layout_prob_horizontal"] = layout_prob.first;
      res["layout_prob_vertical"] = layout_prob.first;
      res["constraints"] = constraints;
    }
    return res;
  }



  std::string ToXml() const {
    std::stringstream ss;
    ss << "<" << views[0].name << "\n";
    ss << "\txmlns:android=\"http://schemas.android.com/apk/res/android\"\n";
    ss << "\txmlns:app=\"http://schemas.android.com/apk/res-auto\"\n";
    ss << "\txmlns:tools=\"http://schemas.android.com/tools\"\n";
    ss << StringPrintf("\t%s=\"%ddp\"\n", Constants::name(Constants::layout_width, Constants::OUTPUT_XML).c_str(), (views[0].xright - views[0].xleft) / 2).c_str();
    ss << StringPrintf("\t%s=\"%ddp\"\n", Constants::name(Constants::layout_height, Constants::OUTPUT_XML).c_str(), (views[0].ybottom - views[0].ytop) / 2).c_str();
//    ss << "\tandroid:layout_width=\"" << (views[0].xright - views[0].xleft) / 2.0 << "dp\"\n";
//    ss << "\tandroid:layout_height=\"" << (views[0].ybottom - views[0].ytop) / 2.0 << "dp\"\n";
    ss << ">\n\n";

    for (const auto& view : views) {
      if (view.is_content_frame()) continue;

      ss << view.ToXml().c_str();
      ss << "\n";
    }

    ss << "</" << views[0].name << ">\n";

    return ss.str();
  }

  void AddView(View&& view) {
    view.pos = views.size();
    views.emplace_back(view);
  }

  const std::vector<View>& GetViews() const {
    return views;
  }

  std::vector<View>& GetViews() {
    return views;
  }

  bool dimensionsMatch(App& other){
	  View& view0 = GetViews()[0];
	  View& otherView0 = GetViews()[0];
	  return view0.xleft == otherView0.xleft && view0.ytop == otherView0.ytop && view0.xright == otherView0.xright && view0.ybottom == otherView0.ybottom;
  }

  void resetViews(){
	  for(View& view: GetViews()){
		  if(view.is_content_frame()){
			  continue;
		  }
		  if(view.id > (int)GetViews().size()/2){
			  view.xleft = -1;
			  view.xright = -1;
			  view.ytop = -1;
			  view.ybottom = -1;
		  }
	  }

  }

  //0 is still at position 0
  std::vector<View> GetViewSortedBySize() const{
	std::vector<View> sorted = views;
	//always start with view id = 0; exclude it here
	std::sort(sorted.begin(), sorted.end(), [] (const View& view1, const View& view2){
		if(view1.is_content_frame()){
			return true;
		}else if(view2.is_content_frame()){
			return false;
		}
		return (view1.width() * view1.height()) > (view2.width() * view2.height());
	});
    return sorted;
  }

  std::vector<int> seq_id_to_pos;

  std::vector<bool> resizable;

private:
  std::vector<View> views;
};


inline std::ostream& operator<<(std::ostream& os, const View& node) {
  os << "View(" << node.id << "), [" << node.xleft << ", " << node.ytop << "], [" << node.xright << ", " << node.ybottom << "], width=" << node.width() << ", height=" << node.height();
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<View>& views) {
  os << "Views:\n";
  for (const View& view : views) {
    os << "\t" << view << "\n";
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const Attribute& attr) {
  os << "prob(" << attr.prob << "), Attr(" << ConstraintTypeStr(attr.type) << "), size(" << ViewSizeStr(attr.view_size) << "), value(" << attr.value_primary << ", " << attr.value_secondary << "), bias(" << attr.bias << "), ";
  if (IsRelationalAnchor(attr.type)) {
    CHECK(attr.tgt_primary != nullptr);
    os << " src(" << attr.src->id << ") -> tgt(" << attr.tgt_primary->id << ")";
  } else {
    CHECK(attr.tgt_primary != nullptr);
    CHECK(attr.tgt_secondary != nullptr);
    os << " src(" << attr.src->id << ") -> tgts(" << attr.tgt_primary->id << ", " << attr.tgt_secondary->id << ")";
  }
  return os;
}



std::unordered_map<std::string, const View> GetNodesWithId(const ProtoScreen& app);

void ForEachRelativeLayoutRelation(const ProtoScreen& app, const std::function<void(const Constants::Name&, const View&, const View&, const std::vector<View>&)>& cb);
void ForEachConstraintLayoutRelation(const ProtoScreen& app, const std::function<void(const Constants::Name&, const View&, const View&, const std::vector<View>&)>& cb);

/************************* Feature Models ***************************/

class AttrModel {
public:
  AttrModel(const std::string& name) : name(name) {

  }

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const {
    return StringPrintf("%s %f, %f", name.c_str(), AttrValue(attr, views), AttrProb(attr, views));
  }

  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const = 0;

  virtual float AttrValue(const Attribute& attr, const std::vector<View>& views) const = 0;

  virtual void AddRelation(const Constants::Name& property, const View& src, const View& tgt, const std::vector<View>& views) = 0;

  virtual void SaveOrDie(FILE* file) const = 0;
  virtual void LoadOrDie(FILE* file) = 0;

  virtual void Dump(std::ostream& os) const = 0;

  static const std::unordered_map<Constants::Name, ConstraintType> ATTRIBUTE_TO_TYPE;
protected:
  const std::string name;


};


class ProbModel {
public:
  ProbModel(const std::string& name) : name(name) {

  }

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const = 0;
  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const = 0;

protected:
  const std::string name;
};

class ModelWrapper : public ProbModel {
public:

  ModelWrapper();

  void AddModel(std::unique_ptr<AttrModel>&& model, double weight) {
    models.push_back(std::move(model));
    weights.push_back(weight);
  }

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const {
    std::string s;
    for (size_t i = 0; i < models.size(); i++) {
      double prob = models[i]->AttrProb(attr, views);
      StringAppendF(&s, "\t\t%f %f weight=%.1f: %s\n", prob, std::log(prob), weights[i], models[i]->DebugProb(attr, views).c_str());
    }
    StringAppendF(&s, "\t\ttotal: %f\n", AttrProb(attr, views));
    return s;
  }

  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    double res = 0;
    for (size_t i = 0; i < models.size(); i++) {
      double prob = models[i]->AttrProb(attr, views);
      res += std::log(prob) * weights[i];
    }
    return res;
  }

  void AddRelation(const Constants::Name& property, const View& src, const View& tgt, const std::vector<View>& views) {
    for (const auto& model : models) {
      model->AddRelation(property, src, tgt, views);
    }
  }

  void SaveOrDie(FILE* file) const {
    for (const auto& model : models) {
      model->SaveOrDie(file);
    }
  }

  void LoadOrDie(FILE* file) {
    for (const auto& model : models) {
      model->LoadOrDie(file);
    }
  }

  void Dump() const {
    for (const auto& model : models) {
      model->Dump(LOG(INFO));
      LOG(INFO) << "\n";
    }
  }


private:
  std::vector<std::unique_ptr<AttrModel>> models;
  std::vector<double> weights;
};


class AttrSizeModel : public AttrModel {
public:

  AttrSizeModel() : AttrModel("size") {
    probs = {
        0.3,
        0.1,
        0.03,
        0.029,
        0.028,
        0.025,
        0.01
    };
  }

  double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    size_t size = AttrValue(attr, views);
    if (size < probs.size()) {
      return probs[size];
    } else {
      return 0.002;
    }
  }

  float AttrValue(const Attribute& attr, const std::vector<View>& views) const {
    return attr.size();
  }

  void AddRelation(const Constants::Name& property, const View& src, const View& tgt, const std::vector<View>& views) {}
  void SaveOrDie(FILE* file) const {}
  void LoadOrDie(FILE* file) {}

  friend std::ostream& operator<<(std::ostream& os, const AttrSizeModel& model);

  void Dump(std::ostream& os) const {
    os << *this;
  }

private:
  std::vector<double> probs;
};

inline std::ostream& operator<<(std::ostream& os, const AttrSizeModel& model) {
  os << model.name << '\n';
  int i = 0;
  for (const auto& prob : model.probs) {
    os << "\t" << i++ << ": " << prob << "\n";
  }
  return os;
}

class CountingModel : public AttrModel {
public:
  CountingModel(const std::string& name,
                const std::vector<std::string>& counter_names,
                const std::unordered_map<ConstraintType, int>& property_to_counter)
      : AttrModel(name), counters_(counter_names.size()), property_to_counter(property_to_counter) {
    for (size_t i = 0; i < counter_names.size(); i++) {
      counters_[i].name = counter_names[i];
    }
  }

  virtual float ValueProto(const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const = 0;
  virtual float ValueAttr(const Attribute& attr, const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const = 0;

  void AddRelation(const Constants::Name& property, const View& src, const View& tgt, const std::vector<View>& views) {
    CHECK(Contains(ATTRIBUTE_TO_TYPE, property));
    ConstraintType type = ATTRIBUTE_TO_TYPE.at(property);
    CHECK(Contains(property_to_counter, type));

    float value = ValueProto(type, src, tgt, views);

    counters_[property_to_counter.at(type)].Add(value);
  }

  double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    if (IsRelationalAnchor(attr.type)) {
      float value = ValueAttr(attr, attr.type, *attr.src, *attr.tgt_primary, views);
      return AttrProbInner(value, attr.type);
    } else {
      const auto& types = SplitCenterAnchor(attr.type);

      float value_l = ValueAttr(attr, types.first, *attr.src, *attr.tgt_primary, views);
      CHECK(attr.tgt_secondary != nullptr);
      float value_r = ValueAttr(attr, types.second, *attr.src, *attr.tgt_secondary, views);

      return (AttrProbInner(value_l, types.first) + AttrProbInner(value_r, types.second)) / 2.0;
    }
  }

  float AttrValue(const Attribute& attr, const std::vector<View>& views) const {
    if (IsRelationalAnchor(attr.type)) {
      return ValueAttr(attr, attr.type, *attr.src, *attr.tgt_primary, views);
    } else {
      const auto& types = SplitCenterAnchor(attr.type);

      float value_l = ValueAttr(attr, types.first, *attr.src, *attr.tgt_primary, views);
      CHECK(attr.tgt_secondary != nullptr);
      float value_r = ValueAttr(attr, types.second, *attr.src, *attr.tgt_secondary, views);

      return (value_l + value_r) / 2.0;
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const CountingModel& node);

  void Dump(std::ostream& os) const {
    os << *this;
  }

  void SaveOrDie(FILE* file) const {
    Serialize::WriteVectorClass(counters_, file);
    Serialize::WriteMap(property_to_counter, file);
  }

  void LoadOrDie(FILE* file) {
    Serialize::ReadVectorClass(&counters_, file);
    Serialize::ReadMap(&property_to_counter, file);
  }

private:
  double AttrProbInner(double value, const ConstraintType& type) const {
    const ValueCounter<float>& counter = counters_[property_to_counter.at(type)];
    return ((double) counter.GetCount(value) + 1.0) / (counter.UniqueValues() + counter.TotalCount());
  }

protected:
  std::vector<ValueCounter<float>> counters_;
  std::unordered_map<ConstraintType, int> property_to_counter;
};


inline std::ostream& operator<<(std::ostream& os, const CountingModel& model) {
  os << model.name << '\n';
  for (const auto& counter : model.counters_) {
    os << "\t" << counter.name << ": total_count(" << counter.TotalCount() << ")\n";
    counter.MostCommon(10, [&os](float value, int count){
      os << "\t\t" << count << ": " << value << "\n";
    });
  }
  return os;
}

class OrientationModel : public CountingModel {
public:
  OrientationModel() : CountingModel("OrientationModel", {
                                         "layout_below",
                                         "layout_above",
                                         "layout_toLeftOf",
                                         "layout_toRightOf",
                                         "layout_alignLeft",
                                         "layout_alignRight",
                                         "layout_alignTop",
                                         "layout_alignBottom"
                                     },{
                                         {ConstraintType::T2B, 0},
                                         {ConstraintType::B2T, 1},
                                         {ConstraintType::R2L, 2},
                                         {ConstraintType::L2R, 3},
                                         {ConstraintType::L2L, 4},
                                         {ConstraintType::R2R, 5},
                                         {ConstraintType::T2T, 6},
                                         {ConstraintType::B2B, 7},
                                     }
  ) {
  }

  float GetAngle(const View& src, const View& tgt, ConstraintType type) const {
    const LineSegment &segment = LineTo(&src, &tgt, type);
    float angle = segment.GetAngle();
    if (std::isnan(angle)) {
      angle = -9999;
    } else {
      angle = std::round(angle);
      if (angle == -180) {
        angle = 180;
      }
    }
    return angle;
  }

  float ValueProto(const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return GetAngle(src, tgt, type);
  }

  float ValueAttr(const Attribute& attr, const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return GetAngle(src, tgt, type);
  }

};

class MarginModel : public CountingModel {
public:
  MarginModel() : CountingModel("MarginModel", {
      "Vertical Margin",
      "Horizontal Margin"
  }, {
      {ConstraintType::T2B, 0},
      {ConstraintType::B2T, 0},
      {ConstraintType::R2L, 1},
      {ConstraintType::L2R, 1},
      {ConstraintType::L2L, 1},
      {ConstraintType::R2R, 1},
      {ConstraintType::T2T, 0},
      {ConstraintType::B2B, 0},
  }) {
    property_to_attribute = {
        {ConstraintType::T2B, Constants::layout_marginTop},
        {ConstraintType::B2T, Constants::layout_marginBottom},
        {ConstraintType::R2L, Constants::layout_marginRight},
        {ConstraintType::L2R, Constants::layout_marginLeft},
        {ConstraintType::L2L, Constants::layout_marginLeft},
        {ConstraintType::R2R, Constants::layout_marginRight},
        {ConstraintType::T2T, Constants::layout_marginTop},
        {ConstraintType::B2B, Constants::layout_marginBottom},
    };
  }

  float GetMargin(const ProtoView* proto_view, const ConstraintType& type) const {
    if (Contains(proto_view->properties(), Constants::name(property_to_attribute.at(type)))) {

      const std::string& value = proto_view->properties().at(Constants::name(property_to_attribute.at(type)));
      if (ValueParser::hasValue(value)) {
        return ValueParser::parseValue(value);
      }
//        LOG(INFO) << "No value in: " << value;

      return 0;
//      return ParseInt32WithDefault(proto_view->properties().at(Constants::name(property_to_attribute.at(type))), 0);
    }

    return 0;
  }

  float ValueProto(const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return GetMargin(src.proto_view, type);
  }

  float ValueAttr(const Attribute& attr, const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return attr.value_primary;
  }

private:
  std::unordered_map<ConstraintType, Constants::Name> property_to_attribute;
};

class DistanceModel : public CountingModel {
public:
  DistanceModel() : CountingModel("DistanceModel", {
      "Vertical Distance",
      "Horizontal Distance"
  }, {
      {ConstraintType::T2B, 0},
      {ConstraintType::B2T, 0},
      {ConstraintType::R2L, 1},
      {ConstraintType::L2R, 1},
      {ConstraintType::L2L, 1},
      {ConstraintType::R2R, 1},
      {ConstraintType::T2T, 0},
      {ConstraintType::B2B, 0},
  }) {
  }

  int GetDistance(const View& src, const View& tgt, const ConstraintType &type) const {
    const LineSegment &segment = LineTo(&src, &tgt, type);
    return std::round(segment.Length());
  }

  float ValueProto(const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return GetDistance(src, tgt, type);
  }

  float ValueAttr(const Attribute& attr, const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return GetDistance(src, tgt, type);
  }

};

class TypeModel : public CountingModel {
public:
  TypeModel() : CountingModel("TypeModel", {
      "Vertical Type",
      "Horizontal Type"
  }, {
      {ConstraintType::T2B, 0},
      {ConstraintType::B2T, 0},
      {ConstraintType::R2L, 1},
      {ConstraintType::L2R, 1},
      {ConstraintType::L2L, 1},
      {ConstraintType::R2R, 1},
      {ConstraintType::T2T, 0},
      {ConstraintType::B2B, 0},
  }) {
  }

  float ValueProto(const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return static_cast<int>(type);
  }

  float ValueAttr(const Attribute& attr, const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return static_cast<int>(type);
  }

};

class IntersectionModel : public CountingModel {
public:
  IntersectionModel() : CountingModel("IntersectionModel", {
      "All Types"
  }, {
      {ConstraintType::T2B, 0},
      {ConstraintType::B2T, 0},
      {ConstraintType::R2L, 0},
      {ConstraintType::L2R, 0},
      {ConstraintType::L2L, 0},
      {ConstraintType::R2R, 0},
      {ConstraintType::T2T, 0},
      {ConstraintType::B2B, 0},
  }) {
  }

  int NumIntersections(const View& src, const View& tgt, const ConstraintType &type, const std::vector<View>& views) const {
    int num_intersections = 0;

    const LineSegment &segment = LineTo(&src, &tgt, type);
    for (const View& view : views) {
      if (view == src || view == tgt) {
        continue;
      }

      if (segment.Intersects(view)) {
        num_intersections++;
      }
    }

    return num_intersections;
  }

  float ValueProto(const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return NumIntersections(src, tgt, type, views);
  }

  float ValueAttr(const Attribute& attr, const ConstraintType& type, const View& src, const View& tgt, const std::vector<View>& views) const {
    return NumIntersections(src, tgt, type, views);
  }

};






template <class Cb>
void ForEachValidApp(const std::string& data_path, const Cb& cb) {
  ValueCounter<std::string> stats;
  ForEachRecord<ProtoApp>(data_path.c_str(), [&](const ProtoApp &app) {
    CHECK_GT(app.screens_size(), 0);

    const ProtoScreen &screen = app.screens(0);
    if (!ValidApp(screen, &stats)) {
      return true;
    }

    App syn_app(screen, true);
    if (syn_app.GetViews().size() == 1) {
      stats.Add("empty layout");
      return true;
    }
    syn_app.InitializeAttributes(screen);

    for (auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
      for (auto &view : syn_app.GetViews()) {
        if (view.is_content_frame()) continue;
        const Attribute& attr = view.attributes.at(orientation);
        if (view.IsCircularRelation(orientation, attr)) {
          stats.Add("Circular relation");
          return true;
        }
      }
    }

    for (auto& view : syn_app.GetViews()) {
      if (view.is_content_frame()) continue;
      if (view.attributes.at(Orientation::HORIZONTAL).view_size == ViewSize::MATCH_PARENT ||
          view.attributes.at(Orientation::VERTICAL).view_size == ViewSize::MATCH_PARENT) {
        stats.Add("depreceated match_parent constraints");
        return true;
      }
    }

    cb(app);
    return true;
  });
  LOG(INFO) << stats;
}

void PrintApp(const App& app, bool with_attributes = true);
void PrintApp(std::stringstream& ss, const App& app, bool with_attributes = true);
void WriteAppToFile(const App& app, std::string name, bool with_attributes = false);

#endif //CC_SYNTHESIS_MODEL_H
