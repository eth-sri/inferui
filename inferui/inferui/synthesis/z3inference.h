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

#ifndef CC_SYNTHESIS_Z3INFERENCE_H
#define CC_SYNTHESIS_Z3INFERENCE_H


#include <regex>
#include <vector>
#include <glog/logging.h>
#include <functional>
#include "z3++.h"
#include "base/stringprintf.h"
#include "base/containerutil.h"
#include "inferui/model/model.h"
#include "inferui/model/constraint_model.h"
#include "inferui/model/synthesis.h"


using namespace z3;

enum class Status {
  SUCCESS = 0,
  UNSAT,
  INVALID,
  TIMEOUT,
  UNKNOWN
};

struct ViewStats{
	int correctMatch = 0;
	int realInCandidates = 0;
	int total = 0;
};

struct Syn_Stats{
	//Watch out: estimates at max 1000 views
	std::vector<ViewStats> view_matching_stats = std::vector<ViewStats>(1000);
    int totalSum = 0;
    int realInCandidatesSum = 0;

	int userCorrectionsSmaller = 0;
	int userCorrectionsBigger = 0;

	int pred_0 = 0;
	int totalPreds = 0;
};

struct SynResult {
public:

  SynResult(App&& app) : app(app), status(Status::INVALID) { }
  SynResult() : status(Status::INVALID) { }

  App app;
  Status status;
  Syn_Stats syn_stats;
};


std::string StatusStr(const Status& status);

inline std::ostream& operator<<(std::ostream& os, const Status& status) {
  os << StatusStr(status);
  return os;
}

class ConstraintData {
public:
  static const std::regex constraint_name_regex;

  ConstraintData(const std::string &value) {
    std::smatch match;
    CHECK(std::regex_search(value, match, constraint_name_regex));

    type = StrToConstraintType(match[1]);
    size = static_cast<ViewSize>(ParseInt32(match[2]));
    orientation = static_cast<Orientation>(ParseInt32(match[3]));
    src = ParseInt32(match[4]);
    primary = ParseInt32(match[5]);
    secondary = (match[6].matched) ? ParseInt32(match[6]) : -1;
  }

  //empty -> is only used as a dummy
  ConstraintData(){}

  ConstraintType type;
  ViewSize size;
  Orientation orientation;
  int src;
  int primary;
  int secondary;
};

inline std::ostream& operator<< (std::ostream& stream, const ConstraintData& data) {
    return stream << " " << ConstraintTypeStr(data.type) << " " << ViewSizeStr(data.size) << ", src pos: " << data.src << ", primary: " << data.primary << ", secondary: " << data.secondary;
}

//view pos
typedef std::map<int, ConstraintData> ConstraintMap;

class AttrScorer {
public:
  AttrScorer(const ProbModel* model, App& app, const Orientation& orientation) : cache(model, app.GetViews(), orientation) {

  }

  int NumConstraints(int view_id) const {
    return cache.NumConstraints(view_id);
  }

  bool IsAllowed(const Attribute* attr) const {
    return cache.IsAllowed(attr);
  }

  void DumpTopN(int view_id, int count, std::vector<View>& views) const {
    cache.DumpTopN(view_id, count, views);
  }

  const Attribute* GetAttr(int view_id, int rank) const {
    return cache.GetAttr(view_id, rank);
  }

  std::pair<int, double> GetRank(const std::string& name, int max_rank) const {
    ConstraintData data(name);
//    LOG(INFO) << name << ", rank: " << cache.GetRank(
//        id_to_pos.at(data.src),
//        data.type,
//        data.size,
//        data.primary,
//        data.secondary
//    );
    return cache.GetRank(
        data.src,
        data.type,
        data.size,
        data.primary,
        data.secondary,
        max_rank
    );
  }

  ConstraintCache cache;
};


class Z3View {
private:
  static int unique_id(int id, const Orientation& orientation) {
    CHECK_LT(id, 1000);
    return id + 1000 * static_cast<int>(orientation);
  }

public:

  Z3View(context& c, int id, const Orientation& orientation, int device_id, int start, int end) : pos(id), start(start), end(end),
                                                      position_start_v(c.int_const(StringPrintf("start_%d_%d", unique_id(id, orientation), device_id).c_str())),
                                                      position_end_v(c.int_const(StringPrintf("end_%d_%d", unique_id(id, orientation), device_id).c_str())),
                                                      margin_start_v(c.int_const(StringPrintf("mstart_%d", unique_id(id, orientation)).c_str())),
                                                      margin_end_v(c.int_const(StringPrintf("mend_%d", unique_id(id, orientation)).c_str())),
//                                                      constraint_start_v(c.int_const(StringPrintf("cstart_%d", id).c_str())),
//                                                      constraint_end_v(c.int_const(StringPrintf("cend_%d", id).c_str()))
                                                      constraints(c), satisfied_id(0), orientation(orientation)
  {
    CHECK_GE(device_id, 0);
  }

  static std::vector<Z3View> ConvertViews(const std::vector<View>& views, const Orientation& orientation, context& c, int device_id = 0) {
    std::vector<Z3View> z3_views;
    for (const View& view : views) {
      CHECK_GE(view.pos, 0);
      if (orientation == Orientation::HORIZONTAL) {
        z3_views.emplace_back(Z3View(c, view.pos, orientation, device_id, view.xleft, view.xright));
      } else {
        z3_views.emplace_back(Z3View(c, view.pos, orientation, device_id, view.ytop, view.ybottom));
      }
    }
    return z3_views;
  }

  expr GetAnchorExpr() {
    return constraints.ctx().int_const(StringPrintf("anchor_%d", unique_id(pos, orientation)).c_str());
  }

  expr GetBiasExpr() const {
    return constraints.ctx().real_const(StringPrintf("bias_%d", unique_id(pos, orientation)).c_str());
  }

  expr GetCostExpr() const {
    return constraints.ctx().real_const(StringPrintf("cost_%d", unique_id(pos, orientation)).c_str());
  }

  void IncSatisfiedId() {
    satisfied_id++;
  }

  expr GetConstraintsSatisfied(context& c) {
    return c.bool_const(StringPrintf("satisfied_%d_%d", unique_id(pos, orientation), satisfied_id).c_str());
  }

  expr GetConstraintsSatisfied() {
    return constraints.ctx().bool_const(StringPrintf("satisfied_%d_%d", unique_id(pos, orientation), satisfied_id).c_str());
  }

  std::string ConstraintName(const ConstraintType& type, const ViewSize& size, const Z3View& other) const {
    return StringPrintf("%s_%d_%d_%d_%d",
                        ConstraintTypeStr(type).c_str(),
                        static_cast<int>(size), static_cast<int>(orientation),
                        pos, other.pos);
  }

  std::string ConstraintName(const ConstraintType& type, const ViewSize& size, const Z3View& l, const Z3View& r) {
    return StringPrintf("%s_%d_%d_%d_%d_%d",
                 ConstraintTypeStr(type).c_str(),
                 static_cast<int>(size), static_cast<int>(orientation),
                        pos, l.pos, r.pos);
  }

  expr AddConstraintExpr(const ConstraintType& type, const ViewSize& size, const Z3View& other) {
    constraints.push_back(constraints.ctx().bool_const(ConstraintName(type, size, other).c_str()));
    return constraints.back();
  }

  expr AddConstraintExpr(const ConstraintType& type, const ViewSize& size, const Z3View& l, const Z3View& r) {
    constraints.push_back(constraints.ctx().bool_const(ConstraintName(type, size, l, r).c_str()));
    return constraints.back();
  }

  bool operator==(const Z3View& other) const {
    return pos == other.pos;
  }

  bool HasFixedPosition() const {
    return start != -1 && end != -1;
  }

  void AssignPosition(model &m) {
    start = m.get_const_interp(position_start_v.decl()).get_numeral_int();
    end = m.get_const_interp(position_end_v.decl()).get_numeral_int();
  }


  int getConstraintRank(model& m, const AttrScorer* scorer) {
    for (size_t i = 0; i < constraints.size(); i++) {
      if (m.get_const_interp(constraints[i].decl()).bool_value() == true) {
    	  return scorer->GetRank(constraints[i].decl().name().str(), -1).first;
      }
    }
    LOG(INFO) << "Error: View is not constrained!";
    return -1;
  }


  ConstraintData AssignModel(model& m, Orientation orientation, std::vector<View>& views, const AttrScorer* scorer = nullptr) {

    for (size_t i = 0; i < constraints.size(); i++) {
      if (m.get_const_interp(constraints[i].decl()).bool_value() == true) {
        ConstraintData data(constraints[i].decl().name().str());
        std::pair<int, int> margins = GetMargins(m, data);
        if (IsCenterAnchor(data.type) && data.size == ViewSize::FIXED) {
          AdjustMargins(&margins);
        }

        if (Contains(views[pos].attributes, orientation)) {
          views[pos].attributes.erase(orientation);
        }

        Attribute attr = Attribute(data.type, data.size, margins.first, margins.second, &views[pos],
                                  &views[data.primary],
                                  (IsCenterAnchor(data.type) ? &views[data.secondary] : nullptr),
                                  GetBias(m));

        if(scorer != nullptr){
        	attr.prob = scorer->GetRank(constraints[i].decl().name().str(), -1).second;
        }

        views[pos].attributes.insert(
            {
                orientation,
				attr

            });

        VLOG(2) << views[pos].attributes.at(orientation);
        return data;
      }
    }
    LOG(INFO) << "Error: View is not constrained!";
    return ConstraintData();
  }

  float GetBias(model& m) {
    float bias;
    std::string value = m.get_const_interp(GetBiasExpr().decl()).get_decimal_string(4);
    if (value.back() == '?') {
      value.pop_back();
    }
    CHECK(ParseFloat(value, &bias)) << value;
    return bias;
  }

  void AdjustMargins(std::pair<int, int>* margins) {
    int min_value = std::min(margins->first, margins->second);
    margins->first -= min_value;
    margins->second -= min_value;
  };

  std::pair<int,int> GetMargins(model& m, const ConstraintData& data) const {
    std::pair<ConstraintType, ConstraintType> types = SplitAnchor(data.type);
    int mstart = 0;
    //TODO: check why margins have very large values that overflow int, is this a bug in Z3?
    if (types.first != ConstraintType::LAST && m.get_const_interp(margin_start_v.decl()).get_numeral_int64() < 10000) {
        mstart = m.get_const_interp(margin_start_v.decl()).get_numeral_int();
    }
    int mend = 0;
    if (types.second != ConstraintType::LAST && m.get_const_interp(margin_end_v.decl()).get_numeral_int64() < 10000) {
        mend = m.get_const_interp(margin_end_v.decl()).get_numeral_int();
    }
    return std::make_pair(mstart, mend);
  }

  const int pos;
  int start;
  int end;

  expr position_start_v;
  expr position_end_v;

  expr margin_start_v;
  expr margin_end_v;

  expr_vector constraints;

  int satisfied_id;
  Orientation orientation;
};

class CandidateConstraints {
public:
  CandidateConstraints(const AttrScorer* scorer, const std::vector<Z3View>& views) :
      scorer(scorer),
      views(views),
      constraints_added(views.size(), 0),
      constraints_max_rank(views.size(), 0),
      constraints_selected(views.size()),
      // contains rank of the first attribute that anchors given view
      anchors(views.size(), -1) {
	  InitAnchor();

  }

  void InitAnchor(){
	    anchors[0] = 0;
	    for (size_t i = 1; i < views.size(); i++) {
	      int num_constraints = scorer->NumConstraints(views[i].pos);
	      //rank 0 is the most likely constraint according to the prob. model
	      for (int rank = 0; rank < num_constraints; rank++) {
	        const Attribute* attr = scorer->GetAttr(views[i].pos, rank);
	        CHECK(attr != nullptr);

	        if (!scorer->IsAllowed(attr) || IsProhibitedConstraint(attr)) {
	          continue;
	        }

	        bool anchor = (attr->tgt_primary->id == 0);
	        if (IsCenterAnchor(attr->type)) {
	          anchor = anchor && (attr->tgt_secondary->id == 0);
	        }

	        if (anchor) {
	          anchors[i] = rank;
	          break;
	        }
	      }
	      CHECK_NE(anchors[i], -1);
	    }
  }

  bool ShouldAdd(const Z3View& view, const std::string& constraint) const;

  void IncreaseRank(int value);

  void IncreaseRank(const Z3View& view, int value);

  void FinishAdding() {
    for (size_t i = 0; i < constraints_added.size(); i++) {
      constraints_added[i] = constraints_max_rank[i];
    }
//    LOG(INFO) << "Constraints Max Rank: " << JoinInts(constraints_max_rank, ',');
  }

  void ResetAdding() {
    for (size_t i = 0; i < constraints_added.size(); i++) {
      constraints_added[i] = 0;
    }
  }

  void ResetAnchor() {
    for (size_t i = 0; i < anchors.size(); i++) {
    	anchors[i] = -1;
    }
  }

  void SetProhibitedConstraintMap(ConstraintMap& _prohibitedConstraints) {
	  prohibitedConstraints = _prohibitedConstraints;
  }

  void RecomputeEntries(){
	  ResetAdding();
	  ResetAnchor();
	  InitAnchor();
  }

  bool IsProhibitedConstraint(const Attribute* attr) const {

	int pos = attr->src->pos;

	ConstraintMap::const_iterator it = prohibitedConstraints.find(pos);
	if(it != prohibitedConstraints.end()){
	   const ConstraintData& data = it->second;

	   bool matchingSecondary = (attr->tgt_secondary == nullptr && data.secondary == -1) || (attr->tgt_secondary != nullptr && attr->tgt_secondary->pos == data.secondary);
	   bool isProhibited = data.type == attr->type && data.size == attr->view_size && attr->tgt_primary->pos == data.primary && matchingSecondary;

	   if(isProhibited){
		   LOG(INFO) << "Prohibited Constraint requested: " << data;
	   }
	   return isProhibited;
	}

     return false;
   }

  void DumpConstraintCounts() {
    std::stringstream ss;
    for (size_t i = 0; i < constraints_selected.size(); i++) {
      ss << constraints_selected[i].size() << " ";
    }
    LOG(INFO) << "Constraint Count: " << ss.str();
  }

  std::vector<const Attribute*> GetAttributes(const Z3View& view);

  const AttrScorer* scorer;
  const std::vector<Z3View>& views;
  std::vector<int> constraints_added;
  std::vector<int> constraints_max_rank;
  std::vector<std::set<int>> constraints_selected;
  ConstraintMap prohibitedConstraints;
  std::vector<int> anchors;
};

struct ConstraintRelationalFixedSize {
public:
  ConstraintRelationalFixedSize(std::pair<ConstraintType, ConstraintType> type, std::function<expr(const Z3View*, const Z3View*, const Z3View*)> fn) : fn_(fn), type_(type) {
  }

  ConstraintType GetType(const Orientation& orientation) const {
    return (orientation == Orientation::HORIZONTAL) ? type_.first : type_.second;
  }

  std::function<expr(const Z3View*, const Z3View*, const Z3View*)> fn_;
  static const std::vector<ConstraintRelationalFixedSize> CONSTRAINTS;

private:
  std::pair<ConstraintType, ConstraintType> type_;
};

struct ConstraintCenteringFixedSize {
public:
  ConstraintCenteringFixedSize(std::pair<ConstraintType, ConstraintType> type, std::function<expr(const Z3View*, const Z3View*, const Z3View*)> fn) : fn_(fn), type_(type) {
  }

  ConstraintType GetType(const Orientation& orientation) const {
    return (orientation == Orientation::HORIZONTAL) ? type_.first : type_.second;
  }

  std::function<expr(const Z3View*, const Z3View*, const Z3View*)> fn_;

  static const std::vector<ConstraintCenteringFixedSize> CONSTRAINTS;
private:
  std::pair<ConstraintType, ConstraintType> type_;
};


struct ConstraintCenteringMatchConstraint {
public:
  ConstraintCenteringMatchConstraint(std::pair<ConstraintType, ConstraintType> type, std::function<expr(const Z3View*, const Z3View*, const Z3View*)> fn) : fn_(fn), type_(type) {
  }

  ConstraintType GetType(const Orientation& orientation) const {
    return (orientation == Orientation::HORIZONTAL) ? type_.first : type_.second;
  }

  std::function<expr(const Z3View*, const Z3View*, const Z3View*)> fn_;

  static const std::vector<ConstraintCenteringMatchConstraint> CONSTRAINTS;
private:
  std::pair<ConstraintType, ConstraintType> type_;
};

struct AttrResolver {
public:
  AttrResolver() {
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[0].fn_);
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[1].fn_);
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[2].fn_);
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[3].fn_);

    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[0].fn_);
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[1].fn_);
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[2].fn_);
    fixed_size_constraints.push_back(ConstraintRelationalFixedSize::CONSTRAINTS[3].fn_);

    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[1].fn_);
    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[0].fn_);
    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[3].fn_);
    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[2].fn_);

    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[1].fn_);
    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[0].fn_);
    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[3].fn_);
    fixed_size_constraints.push_back(ConstraintCenteringFixedSize::CONSTRAINTS[2].fn_);

    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[1].fn_);
    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[0].fn_);
    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[3].fn_);
    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[2].fn_);

    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[1].fn_);
    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[0].fn_);
    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[3].fn_);
    dynamic_size_constraints.push_back(ConstraintCenteringMatchConstraint::CONSTRAINTS[2].fn_);
  }

  const std::function<expr(const Z3View*, const Z3View*, const Z3View*)>& ResolveAttr(const Attribute& attr) {
    if (attr.view_size == ViewSize::FIXED) {
      CHECK_LT(static_cast<int>(attr.type), fixed_size_constraints.size());
      CHECK_GE(static_cast<int>(attr.type), 0);
//      LOG(INFO) << "resolve: " << static_cast<int>(attr.type) << ", size: " << fixed_size_constraints.size();
      return fixed_size_constraints[static_cast<int>(attr.type)];
    } else {
      CHECK_LT(static_cast<int>(attr.type) - 8, dynamic_size_constraints.size());
      CHECK_GE(static_cast<int>(attr.type) - 8, 0);
//      LOG(INFO) << "resolve: " << (static_cast<int>(attr.type) - 8) << ", size: " << dynamic_size_constraints.size();
      return dynamic_size_constraints[static_cast<int>(attr.type) - 8];
    }
  }

private:
  std::vector<std::function<expr(const Z3View*, const Z3View*, const Z3View*)>> fixed_size_constraints;
  std::vector<std::function<expr(const Z3View*, const Z3View*, const Z3View*)>> dynamic_size_constraints;
};


App ResizeApp(
    const App& ref,
    const Device& ref_device,
    const Device& target_device);

App ResizeApp(const App& ref, float ratio);




class BlockingConstraintsHelper {
public:
  BlockingConstraintsHelper(const std::vector<App>& device_apps) {
    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      for (size_t view_id = 1; view_id < device_apps[device_id].GetViews().size(); view_id++) {
        if (!device_apps[device_id].GetViews()[view_id].HasFixedPosition()) {
          empty_view_indices.emplace_back(device_id, view_id);
        }
      }
    }
  }

  void AddViews(const std::vector<App>& device_apps) {
    std::vector<View> views;
    for (const auto& entry : empty_view_indices) {
      const View& view = device_apps[entry.first].GetViews()[entry.second];
      views.push_back(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
    }
    blocking_views.push_back(std::move(views));
  }

  void ResetViews(std::vector<App>& device_apps) const {
    for (const auto& entry : empty_view_indices) {
      View &view = device_apps[entry.first].GetViews()[entry.second];
      view.xleft = -1;
      view.xright = -1;
      view.ytop = -1;
      view.ybottom = -1;
    }
  }

  template <class Solver>
  void AddBlockingConstraints(
      std::vector<OrientationContainer<std::vector<Z3View>>>& z3_views_devices_all, Solver& s) const {
    for (const auto& views : blocking_views) {
      CHECK_EQ(empty_view_indices.size(), views.size());

      expr constraint = s.ctx().bool_val(true);
      for (size_t i = 0; i < empty_view_indices.size(); i++) {
        const auto& blocking_view = views[i];
        const auto& z3_views_devices = z3_views_devices_all[empty_view_indices[i].first];

        constraint = constraint &&
                     (blocking_view.xleft == z3_views_devices[Orientation::HORIZONTAL][empty_view_indices[i].second].position_start_v &&
                      blocking_view.xright == z3_views_devices[Orientation::HORIZONTAL][empty_view_indices[i].second].position_end_v &&
                      blocking_view.ytop == z3_views_devices[Orientation::VERTICAL][empty_view_indices[i].second].position_start_v &&
                      blocking_view.ybottom == z3_views_devices[Orientation::VERTICAL][empty_view_indices[i].second].position_end_v);
      }

      // prevent synthesizing same position for all views as before
//      LOG(INFO) << "Blocking Constraint:";
//      LOG(INFO) << (!constraint).simplify();
      s.add((!constraint).simplify());
    }
  }

private:
  // <device_id, view_id>
  std::vector<std::pair<size_t, size_t>> empty_view_indices;

  std::vector<std::vector<View>> blocking_views;
};


class FullSynthesis {
public:

  Status SynthesizeLayout(App& app) const {
    Status status = Synthesize(app, Orientation::HORIZONTAL);
    if (status == Status::SUCCESS) {
      status = Synthesize(app, Orientation::VERTICAL);
    }
//    if (status == Status::SUCCESS) {
//      PrintApp(app);
//    }
    return status;
  }

  Status SynthesizeLayoutMultiDevice(
      App& app,
      const Device& ref_device,
      const std::vector<Device>& devices) const;

  Status SynthesizeLayoutMultiDevice(
      App& app,
      const ProbModel* model,
      const Device& ref_device,
      const std::vector<Device>& devices) const;

  Status SynthesizeLayoutMultiDeviceProb(
      App& app,
      const ProbModel* model,
      const Device& ref_device,
      const std::vector<Device>& devices,
      bool opt) const;

  Status SynthesizeLayoutMultiDeviceProb(
      App& app,
      const ProbModel* model,
      const Device& ref_device,
      std::vector<App>& device_apps,
      bool opt) const;

  Status SynthesizeLayoutProbOracle(
      App& app,
      const ProbModel* model,
      const Device& ref_device,
	  const std::vector<Device>& device_apps,
      bool opt,
	  const std::string oracleType,
	  const std::string dataset,
	  std::vector<App> debugApps,
	  std::string filename,
	  Syn_Stats& syn_stats,
	  const Json::Value& targetXML,
	  //max number of CEGIS iterations, if the number is bigger than device_apps.size() the minimum is taken, since we must add one new view specification per iteration
	  int maxNumberOfCegisIterations = 2
	  //how many candidates should be sent to Oracle in each CEGIS iteration
  	  ) const; //numberOfCandidates * numberOfCandidates will be generated

  std::pair<check_result, std::vector<Z3View*> > checkSatIntermediate(solver& s, std::vector<Z3View>& z3views) const;

  Status computeCandidates(
  		int numberOfCandidates,
  		std::vector<App>& candidates,
  		std::vector<std::vector<App>>& candidates_resized,
  		const App& app,
  		const std::vector<App>& device_apps,
  		const Device& ref_device,
  		const std::vector<Device>& devices,
  		const AttrScorer* scorer_vertical,
  		const AttrScorer* scorer_horizontal,
  		Timer& timer, bool opt) const;

  Status SynthesizeLayoutMultiDeviceProbUser(
      App& app,
      const ProbModel* model,
      const Device& ref_device,
      std::vector<App>& device_apps,
      bool opt,
      bool robust,
      const std::function<bool(const App&)>& cb) const;

  Status SynthesizeLayoutMultiAppsProb(
      App& app,
      const ProbModel* model,
      const Device& ref_device,
      std::vector<App>& device_apps,
      bool opt) const;


  Status SynthesizeLayoutIterative(
      App& app,
      const ProbModel* model,
      std::vector<App>& device_apps,
      bool opt,
      int max_candidates,
      const std::function<bool(int, const App&, const std::vector<App>&)>& candidate_cb,
      const std::function<std::vector<App>(int, const App&)>& predict_cb,
      const std::function<void(int)>& iter_cb) const;

  Status SynthesizeLayoutMultiAppsProbSingleQueryCandidates(
      App& app,
      const ProbModel* model,
      std::vector<App>& device_apps,
      bool opt,
      const std::function<bool(const App&, const std::vector<App>&)>& cb) const;

  Status SynthesizeLayoutMultiAppsProbSingleQuery(
      App& app,
      const ProbModel* model,
      std::vector<App>& device_apps,
      bool opt) const;

//private:
  template <class S>
  void AddPositionConstraints(S& s, std::vector<Z3View>& views, bool fixed_bias = false) const {
    for (Z3View& view : views) {
      s.add(view.margin_start_v >= 0);
      s.add(view.margin_end_v >= 0);

      s.add(view.position_start_v == view.start);
      s.add(view.position_end_v == view.end);

      if (fixed_bias) {
        s.add(view.GetBiasExpr() == s.ctx().real_val("0.5"));
      } else {
        s.add(view.GetBiasExpr() >= 0);
        s.add(view.GetBiasExpr() <= 1);
      }
    }
  }

  template <class S, class Fn>
  void ForEachNonRootView(
      S& s,
      const Orientation& orientation,
      std::vector<Z3View>& views,
      const Fn& fn,
      const std::function<bool(const std::string&, const Z3View&)>& filter) const {
    for (Z3View& view : views) {
      if (view.pos == 0) {
        continue;
      }

      fn(s, orientation, view, views, filter);
    }
  }

  template <class S>
  static void AddFixedSizeRelationalConstraint(
      S& s,
      const Orientation& orientation,
      Z3View& src,
      std::vector<Z3View>& views,
      const std::function<bool(const std::string&, const Z3View&)>& filter) {
    for (Z3View& tgt : views) {
      if (src == tgt) continue;

      for (const auto& constraint : ConstraintRelationalFixedSize::CONSTRAINTS) {

        if (tgt.pos == 0 && (constraint.GetType(orientation) == ConstraintType::T2B || constraint.GetType(orientation) == ConstraintType::B2T ||
            constraint.GetType(orientation) == ConstraintType::L2R || constraint.GetType(orientation) == ConstraintType::R2L)) {
          continue;
        }

        if (!filter(src.ConstraintName(constraint.GetType(orientation), ViewSize::FIXED, tgt), src)) {
          continue;
        }

        expr cond = src.AddConstraintExpr(constraint.GetType(orientation), ViewSize::FIXED, tgt);
        expr value = constraint.fn_(&src, &tgt, nullptr);

        s.add(implies(cond, value && src.GetAnchorExpr() == tgt.GetAnchorExpr() + 1));
      }
    }

  }

  template <class S>
  static void AddFixedSizeCenteringConstraint(
      S& s,
      const Orientation& orientation,
      Z3View& src,
      std::vector<Z3View>& views,
      const std::function<bool(const std::string&, const Z3View&)>& filter) {
    Product<Z3View>(views, [&views, &src, &s, &orientation, &filter](Z3View& l, Z3View& r) {
      if (l == src || r == src) return;

      for (const auto& constraint : ConstraintCenteringFixedSize::CONSTRAINTS) {
        if ((constraint.GetType(orientation) == ConstraintType::L2RxR2L || constraint.GetType(orientation) == ConstraintType::T2BxB2T) && l == r) {
          continue;
        }

        if (l.pos == 0 && r.pos == 0 && (constraint.GetType(orientation) == ConstraintType::L2LxR2L || constraint.GetType(orientation) == ConstraintType::L2RxR2R ||
            constraint.GetType(orientation) == ConstraintType::T2TxB2T || constraint.GetType(orientation) == ConstraintType::T2BxB2B)) {
          continue;
        }

        if (!filter(src.ConstraintName(constraint.GetType(orientation), ViewSize::FIXED, l, r), src)) {
          continue;
        }

        expr cond = src.AddConstraintExpr(constraint.GetType(orientation), ViewSize::FIXED, l, r);

        expr value = constraint.fn_(&src, &l, &r);
        s.add(implies(cond, value && src.GetAnchorExpr() == l.GetAnchorExpr() + r.GetAnchorExpr() + 1));
      }
    });
  }


  template <class S>
  static void AddMatchConstraintCenteringConstraint(
      S& s,
      const Orientation& orientation,
      Z3View& src,
      std::vector<Z3View>& views,
      const std::function<bool(const std::string&, const Z3View&)>& filter) {
    Product<Z3View>(views, [&views, &src, &s, &orientation, &filter](Z3View& l, Z3View& r) {
      if (l == src || r == src) return;

      for (const auto& constraint : ConstraintCenteringMatchConstraint::CONSTRAINTS) {
        if ((constraint.GetType(orientation) == ConstraintType::L2RxR2L || constraint.GetType(orientation) == ConstraintType::T2BxB2T) && l == r) {
          continue;
        }

        if (l.pos == 0 && r.pos == 0 && (constraint.GetType(orientation) == ConstraintType::L2LxR2L || constraint.GetType(orientation) == ConstraintType::L2RxR2R ||
                                       constraint.GetType(orientation) == ConstraintType::T2TxB2T || constraint.GetType(orientation) == ConstraintType::T2BxB2B)) {
          continue;
        }

        if (!filter(src.ConstraintName(constraint.GetType(orientation), ViewSize::MATCH_CONSTRAINT, l, r), src)) {
          continue;
        }


        expr cond = src.AddConstraintExpr(constraint.GetType(orientation), ViewSize::MATCH_CONSTRAINT, l, r);

        expr value = constraint.fn_(&src, &l, &r);
        s.add(implies(cond, value && src.GetAnchorExpr() == l.GetAnchorExpr() + r.GetAnchorExpr() + 1));
      }
    });
  }

  template <class S>
  void FinishedAddingConstraints(S& s, std::vector<Z3View>& views) const {
    //ensure that exactly one constraint is selected
    for (Z3View& view : views) {
      if (view.pos == 0) continue;

//      LOG(INFO) << "FinishedAddingConstraints>>View " << view.id << " has " << view.constraints.size() << " constraints.";
      std::vector<int> coeffs(view.constraints.size(), 1);
      CHECK_GE(view.constraints.size(), 1);
      s.add(implies(view.GetConstraintsSatisfied(), pbeq(view.constraints, coeffs.data(), 1)));

    }
  }

  template <class S>
  void FinishedAddingConstraints(S& s, Z3View& view) const {
      std::vector<int> coeffs(view.constraints.size(), 1);
      CHECK_GE(view.constraints.size(), 1);
      s.add(implies(view.GetConstraintsSatisfied(), pbeq(view.constraints, coeffs.data(), 1)));
  }

  template <class S>
  void AddAnchorConstraints(S& s, std::vector<Z3View>& views) const {
    for (Z3View& view : views) {
      if (view.pos == 0) {
        s.add(view.GetAnchorExpr() == 0);
      } else {
        s.add(view.GetAnchorExpr() > 0);
      }
    }
  }

  template <class S>
  void addBlockingConstraints(S& s,std::vector<Z3View>& z3_views,  context& c) const{

	  s.add(z3_views[0].position_start_v == z3_views[0].start);
  	  s.add(z3_views[0].position_end_v == z3_views[0].end);

  	  expr x = c.bool_const("x_empty_expr");
  	  expr old_positions = x;
  	  for (Z3View& view : z3_views) {
  	      if (view.pos == 0) continue;
  	      old_positions = old_positions && view.position_start_v == view.start && view.position_end_v == view.end;
  	  }
  	  s.add(x);
  	  s.add(!(old_positions));

  }

  Status Synthesize(App& app, const Orientation& orientation) const;

  template <class S>
  static void AssertNotOutOfBounds(S& s, std::vector<Z3View>& views) {
    // Ensure that views are not outside of the screen
    const Z3View& root = views[0];
    for (Z3View& view : views) {
      if (view.pos == 0) continue;
      if (view.HasFixedPosition()) continue;
      s.add(view.position_start_v >= root.position_start_v);
//          LOG(INFO) << view.position_start_v << " >= " << root.position_start_v;
      s.add(view.position_end_v <= root.position_end_v);
//          LOG(INFO) << view.position_end_v << " <= " << root.position_end_v;
    }
  }

  template <class S>
  static void AssertKeepsIntersection(S& s, const App& ref, std::vector<Z3View>& z3_ref_views, std::vector<Z3View>& z3_app_views) {
    CHECK_EQ(z3_ref_views.size(), z3_app_views.size());
    for (size_t i = 1; i < z3_ref_views.size(); i++) {
      for (size_t j = i + 1; j < z3_ref_views.size(); j++) {
        Z3View& src_ref = z3_ref_views[i];
        Z3View& tgt_ref = z3_ref_views[j];
        if (src_ref.HasFixedPosition() && tgt_ref.HasFixedPosition()) continue;

        Z3View& src_app = z3_app_views[i];
        Z3View& tgt_app = z3_app_views[j];

        if (NumIntersections(ref.GetViews()[i], ref.GetViews()[j], ref.GetViews()) > 0) continue;

        if (src_ref.start == tgt_ref.start) {
          s.add(src_app.position_start_v == tgt_app.position_start_v);
        } else if (src_ref.start < tgt_ref.start) {
          s.add(src_app.position_start_v < tgt_app.position_start_v);
        } else {
          s.add(src_app.position_start_v > tgt_app.position_start_v);
        }

        if (src_ref.start == tgt_ref.end) {
          s.add(src_app.position_start_v == tgt_app.position_end_v);
        } else if (src_ref.start < tgt_ref.end) {
          s.add(src_app.position_start_v < tgt_app.position_end_v);
        } else {
          s.add(src_app.position_start_v > tgt_app.position_end_v);
        }

        if (src_ref.end == tgt_ref.start) {
          s.add(src_app.position_end_v == tgt_app.position_start_v);
        } else if (src_ref.end < tgt_ref.start) {
          s.add(src_app.position_end_v < tgt_app.position_start_v);
        } else {
          s.add(src_app.position_end_v > tgt_app.position_start_v);
        }

        if (src_ref.end == tgt_ref.end) {
          s.add(src_app.position_end_v == tgt_app.position_end_v);
        } else if (src_ref.end < tgt_ref.end) {
          s.add(src_app.position_end_v < tgt_app.position_end_v);
        } else {
          s.add(src_app.position_end_v > tgt_app.position_end_v);
        }

      }
    }
  }

  static std::pair<int, int> AlignmentPoints(int xleft, int xright, int yleft, int yright);

  template <class S>
  static void AssertKeepsMargins(S& s, const App& ref, std::vector<Z3View>& z3_ref_views, std::vector<Z3View>& z3_app_views) {
    CHECK_EQ(z3_ref_views.size(), z3_app_views.size());
    std::set<int> margins = {0, 8, 14, 16, 20, 24, 30, 32, 48};
    for (size_t i = 1; i < z3_ref_views.size(); i++) {
      for (size_t j = 0; j < i; j++) {

        Z3View &src_ref = z3_ref_views[i];
        Z3View &tgt_ref = z3_ref_views[j];
        if (src_ref.HasFixedPosition() && tgt_ref.HasFixedPosition()) continue;

        std::pair<int, int> points = AlignmentPoints(src_ref.start, src_ref.end, tgt_ref.start, tgt_ref.end);
//      LOG(INFO) << "Keep Margins: " << i << " " << j << ", margin: " << points.first << " - " << points.second << " = " << std::abs(points.first - points.second);
//      LOG(INFO) << src_ref.start << ", " << src_ref.end << " -- " << tgt_ref.start << ", " << tgt_ref.end;
        if (points.first == -1 || !Contains(margins, std::abs(points.first - points.second)) ||
            NumIntersections(ref.GetViews()[i], ref.GetViews()[j], ref.GetViews()) > 0) {
          continue;
        }

//      LOG(INFO) << "Keep Margins: " << src_ref.id << " " << tgt_ref.id << ", margin: " << points.first << " - " << points.second << " = " << std::abs(points.first - points.second);
//      LOG(INFO) << src_ref.start << ", " << src_ref.end << " -- " << tgt_ref.start << ", " << tgt_ref.end;
        Z3View &src_app = z3_app_views[i];
        Z3View &tgt_app = z3_app_views[j];

        if (src_ref.start == points.first && tgt_ref.start == points.second) {
          s.add(src_ref.position_start_v - tgt_ref.position_start_v == src_app.position_start_v - tgt_app.position_start_v);
        } else if (src_ref.start == points.first && tgt_ref.end == points.second) {
          s.add(src_ref.position_start_v - tgt_ref.position_end_v == src_app.position_start_v - tgt_app.position_end_v);
        } else if (src_ref.end == points.first && tgt_ref.start == points.second) {
          s.add(src_ref.position_end_v - tgt_ref.position_start_v == src_app.position_end_v - tgt_app.position_start_v);
        } else {
          s.add(src_ref.position_end_v - tgt_ref.position_end_v == src_app.position_end_v - tgt_app.position_end_v);
        }

      }
    }
  }

  template <class S>
  static void AssertKeepsSizeRatio(S& s, const App& ref, std::vector<Z3View>& z3_ref_views, std::vector<Z3View>& z3_app_views, const App& app) {

    std::set<std::pair<int, int>> ratios_frac = {
        std::make_pair(1,1),
        std::make_pair(3,4),
        std::make_pair(4,3),
        std::make_pair(9,16),
        std::make_pair(16,9)
    };
    for (size_t i = 1; i < z3_ref_views.size(); i++) {
      const View& view = ref.GetViews()[i];
      if (z3_ref_views[i].HasFixedPosition()) continue;

      for (const auto& ratio : ratios_frac) {
        if (view.width() * ratio.first != view.height() * ratio.second) continue;

        Z3View &horizontal_view = z3_app_views[i];
        const View &vertical_view = app.GetViews()[i];
        s.add((horizontal_view.position_end_v - horizontal_view.position_start_v) * ratio.first == (vertical_view.ybottom - vertical_view.ytop) * ratio.second);

        break;
      }

    }
  }

  template <class S>
  static void AssertKeepsCentering(S& s, const App& app, std::vector<Z3View>& z3_ref_views, std::vector<Z3View>& z3_app_views) {
    CHECK_EQ(z3_ref_views.size(), z3_app_views.size());

    for (size_t i = 1; i < z3_ref_views.size(); i++) {
      Z3View& src_ref = z3_ref_views[i];
      Z3View& src_app = z3_app_views[i];


      //Centering with content frame
      if (src_ref.start + src_ref.end == z3_ref_views[0].start + z3_ref_views[0].end) {
//        LOG(INFO) << "Centering: " << src_ref.id << " (" << (src_app.start + src_app.end) << "), "
//                  << z3_ref_views[0].id << " (" << (z3_ref_views[0].start + z3_ref_views[0].end) << ")";

        s.add(src_app.position_start_v + src_app.position_end_v == z3_app_views[0].position_start_v + z3_app_views[0].position_end_v);
      }

      //Centering with single other view
      for (size_t l = i + 1; l < z3_ref_views.size(); l++) {
        if (i == l) continue;

        Z3View& tgt_ref = z3_ref_views[l];
        Z3View& tgt_app = z3_app_views[l];
        if (src_ref.HasFixedPosition() && tgt_ref.HasFixedPosition()) continue;

        if (src_ref.start + src_ref.end == tgt_ref.start + tgt_ref.end) {
//          LOG(INFO) << "Centering: " << src_ref.id << " (" << (src_app.start + src_app.end) << "), "
//                    << tgt_app.id << " (" << (tgt_app.start + tgt_app.end) << ")";

          s.add(src_app.position_start_v + src_app.position_end_v == tgt_app.position_start_v + tgt_app.position_end_v);
        }
      }

      //centering in between two views
      for (size_t l = 0; l < z3_ref_views.size(); l++) {
        if (i == l) continue;

        Z3View& l_ref = z3_ref_views[l];
        Z3View& l_app = z3_app_views[l];
        for (size_t r = l + 1; r < z3_ref_views.size(); r++) {
          if (i == r) continue;

          Z3View& r_ref = z3_ref_views[r];
          Z3View& r_app = z3_app_views[r];

          if (NumIntersections(app.GetViews()[i], app.GetViews()[l], app.GetViews()) > 0 ||
              NumIntersections(app.GetViews()[i], app.GetViews()[r], app.GetViews()) > 0
//              NumIntersections(app.views[l], app.views[r], app.views) > 0
              ) continue;



          if (src_ref.start + src_ref.end == l_ref.start + r_ref.end) {

//            LOG(INFO) << "Centering start-end; " << src_ref.id << " (" << src_app.start << ", " << src_app.end << "), "
//                      << l_ref.id << ", " << r_ref.id << " (" << l_app.start << ", " << r_app.end << ")";

            s.add(src_app.position_start_v + src_app.position_end_v == l_app.position_start_v + r_app.position_end_v);
          } else if (src_ref.start + src_ref.end == l_ref.end + r_ref.start) {

//            LOG(INFO) << "Centering end-start; " << src_ref.id << " (" << src_app.start  << ", " <<  src_app.end << "), "
//                      << l_ref.id << ", " << r_ref.id << " (" << l_app.end << ", " << r_app.start << ")";

            s.add(src_app.position_start_v + src_app.position_end_v == l_app.position_end_v + r_app.position_start_v);
          }
        }
      }

    }
  }


  bool ValidConstraint(const ConstraintType& type,
                       const Z3View* src, const Z3View* l, const Z3View* r) const {
    if (IsRelationalAnchor(type)) {
      return !(l->pos == 0 && (type == ConstraintType::T2B || type == ConstraintType::B2T ||
          type == ConstraintType::L2R || type == ConstraintType::R2L));
    } else {
      if ((type == ConstraintType::L2RxR2L || type == ConstraintType::T2BxB2T) && *l == *r) {
        return false;
      }

      if (l->pos == 0 && r->pos == 0 && (type == ConstraintType::L2LxR2L || type == ConstraintType::L2RxR2R ||
                                       type == ConstraintType::T2TxB2T || type == ConstraintType::T2BxB2B)) {
        return false;
      }

      return true;
    }
  }

  Z3View* findView(const View* view, std::vector<Z3View>& z3_views) const {
    if (view == nullptr) return nullptr;
    return &z3_views[view->pos];
    LOG(FATAL) << "View not found!";
  }

  template <class S>
  void AddSynAttributes(S& s,
                        std::vector<Z3View>& z3_views,
                        const Orientation& orientation,
                        CandidateConstraints& candidates, bool opt = false) const {
    AttrResolver resolver;
    for (Z3View& view : z3_views) {
      if (view.pos == 0) continue;
      for (const Attribute* attr : candidates.GetAttributes(view)) {

        Z3View* src = findView(attr->src, z3_views);
        Z3View* l = findView(attr->tgt_primary, z3_views);
        Z3View* r = findView(attr->tgt_secondary, z3_views);
        CHECK(ValidConstraint(attr->type, src, l, r));

        expr value = resolver.ResolveAttr(*attr)(src, l, r);
        if (IsRelationalAnchor(attr->type)) {
          expr cond = src->AddConstraintExpr(attr->type, ViewSize::FIXED, *l);
          s.add(implies(cond, value && src->GetAnchorExpr() == l->GetAnchorExpr() + 1));
          if (opt) {
            s.add(implies(cond, src->GetCostExpr() == s.ctx().real_val(StringPrintf("%f", attr->prob).c_str())));
          }
        } else {
          expr cond = src->AddConstraintExpr(attr->type, attr->view_size, *l, *r);
          s.add(implies(cond, value && src->GetAnchorExpr() == l->GetAnchorExpr() + r->GetAnchorExpr() + 1));
          if (opt) {
            s.add(implies(cond, src->GetCostExpr() == s.ctx().real_val(StringPrintf("%f", attr->prob).c_str())));
          }
        }

      }

    }
  }

  template <class S>
  void AddSynConstraints(S& s, std::vector<Z3View>& z3_views, const Orientation& orientation,
                         const CandidateConstraints& candidates, bool opt = false) const {
    ForEachNonRootView(s, orientation, z3_views, AddFixedSizeRelationalConstraint<S>,
                       [&candidates, &s, &opt](const std::string& name, const Z3View& src) {
                         if (!candidates.ShouldAdd(src, name)) return false;
//                         LOG(INFO) << "Adding: " << name;

                       if (opt) {
                         expr cond = s.ctx().bool_const(name.c_str());
                         s.add(implies(cond, src.GetCostExpr() == s.ctx().real_val(StringPrintf("%f", candidates.scorer->GetRank(name, -1).second).c_str())));
                       }
//                         LOG(INFO) << candidates.ShouldAdd(src, name) << " " << rank.first << " " << name;
//                         return rank.first < 10;
                         return true;
                       });

    ForEachNonRootView(s, orientation, z3_views, AddFixedSizeCenteringConstraint<S>,
                       [&candidates, &s, &opt](const std::string &name, const Z3View &src) {
                         if (!candidates.ShouldAdd(src, name)) return false;


                         if (opt) {
                           expr cond = s.ctx().bool_const(name.c_str());
                           s.add(implies(cond, src.GetCostExpr() == s.ctx().real_val(StringPrintf("%f", candidates.scorer->GetRank(name, -1).second).c_str())));
                         }

                         return true;
                       });

    ForEachNonRootView(s, orientation, z3_views, AddMatchConstraintCenteringConstraint<S>,
                       [&candidates, &s, &opt](const std::string& name, const Z3View& src){
                         if (!candidates.ShouldAdd(src, name)) return false;

                         if (opt) {
                           expr cond = s.ctx().bool_const(name.c_str());
                           s.add(implies(cond, src.GetCostExpr() == s.ctx().real_val(StringPrintf("%f", candidates.scorer->GetRank(name, -1).second).c_str())));
                         }

                         return true;
                       });
  }

  template <class S>
  void AddGenAttributes(S& s,
                        std::vector<Z3View>& z3_views, const App& app,
                        const Orientation& orientation,
                        CandidateConstraints& candidates) const {
    AttrResolver resolver;
    for (Z3View& view : z3_views) {
      if (view.pos == 0) continue;

//      LOG(INFO) << "View: " << view.pos;
      for (const Attribute* attr : candidates.GetAttributes(view)) {
        Z3View* src = findView(attr->src, z3_views);
        Z3View* l = findView(attr->tgt_primary, z3_views);
        Z3View* r = findView(attr->tgt_secondary, z3_views);
//        LOG(INFO) << ValidConstraint(attr->type, src, l, r) << ", " << *attr;
        ValidConstraint(attr->type, src, l, r);

        expr value = resolver.ResolveAttr(*attr)(src, l, r);
        expr cond = IsRelationalAnchor(attr->type)
                    ? src->AddConstraintExpr(attr->type, ViewSize::FIXED, *l)
                    : src->AddConstraintExpr(attr->type, attr->view_size, *l, *r);

        if (attr->view_size == ViewSize::FIXED) {
          int size = (orientation == Orientation::HORIZONTAL) ? app.GetViews()[src->pos].width()
                                                               : app.GetViews()[src->pos].height();
          s.add(implies(cond, value && src->position_start_v + size == src->position_end_v));
        } else {
          s.add(implies(cond, value && src->position_end_v - src->position_start_v >= 0));
        }

      }

    }
  }

  template <class S>
  void AddGenConstraints(S& s, std::vector<Z3View>& z3_views_device, const App& app, const Orientation& orientation,
                         const CandidateConstraints& candidates) const {
    ForEachNonRootView(s, orientation, z3_views_device, AddFixedSizeRelationalConstraint<solver>,
                       [&s, &app, &orientation, &candidates](const std::string &name, const Z3View &src) {
                         if (!candidates.ShouldAdd(src, name)) return false;

                         expr cond = s.ctx().bool_const(name.c_str());
                         int value = (orientation == Orientation::HORIZONTAL) ? app.GetViews()[src.pos].width()
                                                                              : app.GetViews()[src.pos].height();
                         s.add(implies(cond, src.position_start_v + value == src.position_end_v));
                         return true;
                       });

    ForEachNonRootView(s, orientation, z3_views_device, AddFixedSizeCenteringConstraint<solver>,
                       [&s, &app, &orientation, &candidates](const std::string &name, const Z3View &src) {
                         if (!candidates.ShouldAdd(src, name)) return false;
                         expr cond = s.ctx().bool_const(name.c_str());
                         int value = (orientation == Orientation::HORIZONTAL) ? app.GetViews()[src.pos].width()
                                                                              : app.GetViews()[src.pos].height();
                         s.add(implies(cond, src.position_start_v + value == src.position_end_v));
                         return true;
                       });

    ForEachNonRootView(s, orientation, z3_views_device, AddMatchConstraintCenteringConstraint<solver>,
                       [&s, &candidates](const std::string &name, const Z3View &src) {

                         if (!candidates.ShouldAdd(src, name)) return false;
                         s.add(implies(cond, src.position_end_v - src.position_start_v >= 0));
                         return true;
                       });
  }

  Status SynthesizeMultiDevice(
      App& app,
      const Orientation& orientation,
      const AttrScorer* scorer,
      const Device& ref_device,
      std::vector<App>& device_apps,
      Timer& timer) const;

  template <class S>
  void AddConstraintsSingleQuery(
      App& app,
      std::vector<App>& device_apps,
      std::vector<OrientationContainer<std::vector<Z3View>>>& z3_views_devices_all,
      OrientationContainer<std::vector<Z3View>>& z3_views_all,
      OrientationContainer<CandidateConstraints>& candidates_all,
      bool opt,
      S& s) const {
    for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
      auto& candidates = candidates_all[orientation];
      auto& z3_views = z3_views_all[orientation];

//      candidates.IncreaseRank(5);

      AddPositionConstraints(s, z3_views, true);
      AddAnchorConstraints(s, z3_views);
      AddSynAttributes(s, z3_views, orientation, candidates, opt);
      //exactly one constraint will be matched
      FinishedAddingConstraints(s, z3_views);

      for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
        std::vector<Z3View>& z3_views_device = z3_views_devices_all[device_id][orientation];

        //if position is fixed/known -> set it
        for (size_t i = 1; i < z3_views.size(); i++) {
          const Z3View& view = z3_views_device[i];
          if (view.HasFixedPosition()) {
            s.add(view.position_start_v == view.start);
            s.add(view.position_end_v == view.end);
//            LOG(INFO) << "User Feedback device(" << device_id << "), view(" << view.pos << ") = [" << view.start << ", " << view.end << "]";
          }
        }

        // Set fixed size of the root layout
        s.add(z3_views_device[0].position_start_v == z3_views_device[0].start);
        s.add(z3_views_device[0].position_end_v == z3_views_device[0].end);

        AddGenAttributes(s, z3_views_device, app, orientation, candidates);
      }

      candidates.FinishAdding();
    }
  }

  Status GetSatConstraintsSingleQuery(
      App& app,
      const OrientationContainer<AttrScorer>& scorers,
      std::vector<App>& device_apps,
      Timer& timer,
      OrientationContainer<CandidateConstraints>& candidates_all,
      BlockingConstraintsHelper* blocking_constraints = nullptr) const;

  std::pair<Status, CandidateConstraints> GetSatConstraints(
      App& app,
      const Orientation& orientation,
      const AttrScorer* scorer,
      const Device& ref_device,
      std::vector<App>& device_apps,
      Timer& timer,
      bool user_input, bool robust) const;

  std::pair<Status, CandidateConstraints> GetSatConstraintsOracle(
      App& app,
      const Orientation& orientation,
      const AttrScorer* scorer,
      const Device& ref_device,
      const std::vector<App>& device_apps,
      Timer& timer,
	  const std::vector<App>& blockedApps
  	) const;

  Status SynthesizeMultiDeviceProbSingleQuery(
      App& app,
      const OrientationContainer<AttrScorer>& scorers,
      std::vector<App>& device_apps,
      Timer& timer,
      bool opt = false,
      BlockingConstraintsHelper* blocking_constraints = nullptr) const;

  Status SynthesizeMultiDeviceProbSingleQueryInner(
      App& app,
      const OrientationContainer<AttrScorer>& scorers,
      std::vector<App>& device_apps,
      Timer& timer,
      bool opt,
      context& c,
      OrientationContainer<CandidateConstraints>& candidates_all,
      BlockingConstraintsHelper* blocking_constraints) const;

  Status SynthesizeMultiDeviceProb(
      App& app,
      const Orientation& orientation,
      const AttrScorer* scorer,
      const Device& ref_device,
      std::vector<App>& device_apps,
      Timer& timer,
      bool user_input,
      bool robust,
      bool opt = false) const;

  Status SynthesizeMultiDeviceProbLarissaTest(
      App& app,
      const Orientation& orientation,
      const AttrScorer* scorer,
      const Device& ref_device,
      std::vector<App>& device_apps,
      Timer& timer,
      bool user_input,
      bool robust) const;

  std::pair<Status, ConstraintMap>  SynthesizeDeviceProbOracle(
      App& app,
      const Orientation& orientation,
      const AttrScorer* scorer,
      const Device& ref_device,
      const std::vector<App>& device_apps,
	  std::vector<App>& target_apps,
      Timer& timer,
      bool opt,
	  const std::vector<App>& blockedApps = std::vector<App>()
  	  ) const;

	int computeMatchings(
			std::vector<App>& candidates,
			std::vector<std::vector<App>>& candidates_resized,
			int deviceId,
			bool checkLayouts = false) const;

};


class LayoutSolver {
public:



  template <class Fn>
  void ForEachNonRootView(
      solver& s,
      const Orientation& orientation,
      std::vector<Z3View>& views,
      const Fn& fn,
      const std::function<bool(const std::string&, const Z3View&)>& filter) const {
    for (Z3View& view : views) {
      if (view.pos == 0) {
        continue;
      }

      fn(s, orientation, view, views, filter);
    }
  }

  std::set<std::string> CollectConstraints(const Orientation& orientation, const App& ref, std::vector<Z3View>& views) const {

    std::set<std::string> res;
    for (size_t i = 0; i < views.size(); i++) {
      Z3View& view = views[i];
      if (view.pos == 0) {
        continue;
      }

      const Attribute& attr = ref.GetViews()[view.pos].attributes.at(orientation);
      std::string name = IsRelationalAnchor(attr.type) ?
                         view.ConstraintName(attr.type, attr.view_size, views[attr.tgt_primary->pos]) :
                         view.ConstraintName(attr.type, attr.view_size, views[attr.tgt_primary->pos], views[attr.tgt_secondary->pos]);
      res.insert(name);
      VLOG(2) << name;
      VLOG(2) << '\t' << attr;
    }
    return res;
  }

  void SetMargins(solver& s, const Orientation& orientation, const App& ref, std::vector<Z3View>& views) const {
    for (size_t i = 0; i < views.size(); i++) {
      Z3View& view = views[i];
      if (view.pos == 0) {
        continue;
      }

      const Attribute& attr = ref.GetViews()[i].attributes.at(orientation);
      s.add(view.margin_start_v == attr.value_primary);
      s.add(view.margin_end_v == attr.value_secondary);

      s.add(view.GetBiasExpr() == s.ctx().real_val(std::to_string(attr.bias).c_str()));
    }
  }

  void SetSize(solver& s, const Orientation& orientation, const App& ref, std::vector<Z3View>& views) const {

    for (size_t i = 0; i < views.size(); i++) {
      Z3View& view = views[i];
      if (view.pos == 0) {
        // root view has fixed size that we want to layout to fill
        s.add(view.position_start_v == view.start);
        s.add(view.position_end_v == view.end);

      } else {
//        LOG(INFO) << "view id: " << view.id << ", mapped_id: " << id_to_pos[i] << ", num attributes: " << ref.views[view.id].attributes.size();
        const Attribute& attr = ref.GetViews()[i].attributes.at(orientation);
        if (attr.view_size == ViewSize::FIXED) {
          int value = (orientation == Orientation::HORIZONTAL) ? ref.GetViews()[i].width() : ref.GetViews()[i].height();
//          LOG(INFO) << "view " << view.id << " size: " << value << " for " << orientation;
          CHECK_GE(value, 0);
          s.add(view.position_start_v + value == view.position_end_v);
        } else {
          s.add(view.position_end_v - view.position_start_v >= 0);
        }
      }
    }
  }

  bool FinishedAddingConstraints(solver& s, std::vector<Z3View>& views) const {
    //ensure that exactly one constraint is selected
    for (Z3View& view : views) {
      if (view.pos == 0) continue;

      if (view.constraints.size() != 1) {
        return false;
      }
      s.add(view.constraints[0]);
    }
    return true;
  }

  Status Layout(App& app, const App& ref, const Orientation& orientation) const {
    context c;
    solver s(c);

    std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

    SetSize(s, orientation, ref, z3_views);
    SetMargins(s, orientation, ref, z3_views);


    std::set<std::string> constraints = CollectConstraints(orientation, ref, z3_views);
//    LOG(INFO) << "Constraints:";
//    for (auto& constraint : constraints) {
//      LOG(INFO) << "\t" << constraint;
//    }

    ForEachNonRootView(s, orientation, z3_views, FullSynthesis::AddFixedSizeRelationalConstraint<solver>, [&constraints](const std::string& name, const Z3View& src){
      return Contains(constraints, name);
    });
    ForEachNonRootView(s, orientation, z3_views, FullSynthesis::AddFixedSizeCenteringConstraint<solver>, [&constraints](const std::string& name, const Z3View& src){
      return Contains(constraints, name);
    });
    ForEachNonRootView(s, orientation, z3_views, FullSynthesis::AddMatchConstraintCenteringConstraint<solver>, [&constraints](const std::string& name, const Z3View& src){
      return Contains(constraints, name);
    });

    if(!FinishedAddingConstraints(s, z3_views)) {
      return Status::INVALID;
    }

//    LOG(INFO) << s;

    //solve
    params p(c);
    p.set(":timeout", 120000u);
//    p.set(":unsat-core", true);
    s.set(p);
    check_result res = s.check();

    if (res != check_result::sat) {
      LOG(INFO) << "\tLayout " << orientation << ": \t" << res;
      return Status::UNSAT;
    }
    model m = s.get_model();

    {
      for (Z3View &view : z3_views) {
        if (view.pos == 0) continue;

        view.AssignPosition(m);
        if (orientation == Orientation::HORIZONTAL) {
          app.GetViews()[view.pos].xleft = view.start;
          app.GetViews()[view.pos].xright = view.end;
        } else {
          app.GetViews()[view.pos].ytop = view.start;
          app.GetViews()[view.pos].ybottom = view.end;
        }
      }
    }

    return Status::SUCCESS;
  }

  std::pair<Status, App> Layout(const App& ref, int xleft, int ytop, int xright, int ybottom) const {
    App app;
    for (const View& view : ref.GetViews()) {
      if (view.is_content_frame()) {
        app.AddView(View(xleft, ytop, xright, ybottom, view.name, view.id));
      } else {
        app.AddView(View(-1, -1, -1, -1, view.name, view.id));
      }
    }

    Status status = Layout(app, ref, Orientation::HORIZONTAL);
    if (status != Status::SUCCESS) {
      return std::make_pair(status, app);
    }
    status = Layout(app, ref, Orientation::VERTICAL);
    return std::make_pair(status, app);
  }
};




class AppProperties {
public:

  static std::vector<expr> GenBoundConditions(context& c, const std::vector<Z3View>& views) {
    std::vector<expr> res;

    const Z3View& root = views[0];
    for (const Z3View& view : views) {
      if (view.pos == 0) continue;

      res.emplace_back(view.position_start_v >= root.position_start_v);
      res.emplace_back(view.position_end_v <= root.position_end_v);
    }

    return res;
  }

  static void AddPositionConstraints(solver& s, std::vector<Z3View>& views){
    for (Z3View& view : views) {
      s.add(view.position_start_v == view.start);
      s.add(view.position_end_v == view.end);
    }
  }

  static bool CheckBounds(const App& app, const Orientation& orientation) {
    context c;
    solver s(c);

    std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

    AddPositionConstraints(s, z3_views);

    for (const expr& e  : GenBoundConditions(c, z3_views)) {
      s.add(e);
    }

    //solve
    params p(c);
    p.set(":timeout", 120000u);
    s.set(p);
    check_result res = s.check();
//    LOG(INFO) << "\tBounds " << orientation << ": \t" << res;

    return res == check_result::sat;
  }

  static bool CheckBounds(const App& app) {
    return CheckBounds(app, Orientation::HORIZONTAL) && CheckBounds(app, Orientation::VERTICAL);
  }

  static bool CheckIntersection(const App& ref, const App& app, const Orientation& orientation) {
    context c;
    solver s(c);

    std::vector<Z3View> z3_ref_views = Z3View::ConvertViews(ref.GetViews(), orientation, c);
    AddPositionConstraints(s, z3_ref_views);

    std::vector<Z3View> z3_app_views = Z3View::ConvertViews(app.GetViews(), orientation, c, 1);
    AddPositionConstraints(s, z3_app_views);

    FullSynthesis::AssertKeepsIntersection(s, ref, z3_ref_views, z3_app_views);

//    LOG(INFO) << s;

    params p(c);
    p.set(":timeout", 120000u);
    s.set(p);
    check_result res = s.check();

    return res == check_result::sat;
  }

  static bool CheckIntersection(const App& ref, const App& app) {
    return CheckIntersection(ref, app, Orientation::HORIZONTAL) && CheckIntersection(ref, app, Orientation::VERTICAL);
  }

  static bool CheckCentering(const App& ref, const App& app, const Orientation& orientation) {
    context c;
    solver s(c);

    std::vector<Z3View> z3_ref_views = Z3View::ConvertViews(ref.GetViews(), orientation, c);
    AddPositionConstraints(s, z3_ref_views);

    std::vector<Z3View> z3_app_views = Z3View::ConvertViews(app.GetViews(), orientation, c, 1);
    AddPositionConstraints(s, z3_app_views);

    FullSynthesis::AssertKeepsCentering(s, ref, z3_ref_views, z3_app_views);

    params p(c);
    p.set(":timeout", 120000u);
    s.set(p);
    check_result res = s.check();

    return res == check_result::sat;
  }

  static bool CheckCentering(const App& ref, const App& app) {
    return CheckCentering(ref, app, Orientation::HORIZONTAL) && CheckCentering(ref, app, Orientation::VERTICAL);
  }

  static bool CheckMargins(const App& ref, const App& app, const Orientation& orientation) {
    context c;
    solver s(c);


    std::vector<Z3View> z3_ref_views = Z3View::ConvertViews(ref.GetViews(), orientation, c);
    AddPositionConstraints(s, z3_ref_views);

    std::vector<Z3View> z3_app_views = Z3View::ConvertViews(app.GetViews(), orientation, c, 1);
    AddPositionConstraints(s, z3_app_views);

    FullSynthesis::AssertKeepsMargins(s, ref, z3_ref_views, z3_app_views);

    params p(c);
    p.set(":timeout", 120000u);
    s.set(p);
    check_result res = s.check();

    return res == check_result::sat;
  }

  static bool CheckMargins(const App& ref, const App& app) {
    return CheckMargins(ref, app, Orientation::HORIZONTAL) && CheckMargins(ref, app, Orientation::VERTICAL);
  }

  static bool CheckSizeRatio(const App& ref, const App& app) {
    context c;
    solver s(c);

    std::vector<Z3View> z3_ref_views = Z3View::ConvertViews(ref.GetViews(), Orientation::HORIZONTAL, c);
    AddPositionConstraints(s, z3_ref_views);

    std::vector<Z3View> z3_app_views = Z3View::ConvertViews(app.GetViews(), Orientation::HORIZONTAL, c, 1);
    AddPositionConstraints(s, z3_app_views);

    FullSynthesis::AssertKeepsSizeRatio(s, ref, z3_ref_views, z3_app_views, app);

    params p(c);
    p.set(":timeout", 120000u);
    s.set(p);
    check_result res = s.check();

    return res == check_result::sat;
  }

  static bool CheckAllProperties(const App& ref, const App& app){
	  	bool valid = true;
	    if (!AppProperties::CheckBounds(ref) || !AppProperties::CheckBounds(app)) {
	      LOG(INFO) << "CheckProperties: Check Bounds False";
	      valid = false;
	    }

	    if (!AppProperties::CheckIntersection(ref, app)) {
	      LOG(INFO) << "CheckProperties: CheckIntersection False";
	      valid = false;
	    }

	    if (!AppProperties::CheckCentering(ref, app)) {
	      LOG(INFO) << "CheckProperties: CheckCentering False";
	      valid = false;
	    }

	    if (!AppProperties::CheckMargins(ref, app)) {
	      LOG(INFO) << "CheckProperties: CheckMargins False";
	      valid = false;
	    }

	    if (!AppProperties::CheckSizeRatio(ref, app)) {
	      LOG(INFO) << "CheckProperties: CheckSizeRatio False";
	      valid = false;
	    }

	    if(app.GetViews().size() != ref.GetViews().size()){
		      LOG(INFO) << "CheckProperties: Different number of views";
		      valid = false;
	    }
	    for(int i = 0; i < (int) app.GetViews().size(); i++){
	    	const View& view = app.GetViews()[i];
	    	const View& view1 = ref.GetViews()[i];

	    	double ratio_width = std::max((double)view.width()/view1.width(), (double) view1.width()/view.width());
	    	double ratio_height = std::max((double)view.height()/view1.height(), (double) view1.height()/view.height());

	    	if(ratio_width > 2.0 || ratio_height > 2.0){
			      LOG(INFO) << "CheckProperties: Anticipated resizing violated";
			      valid = false;
	    	}

	    }
	    return valid;
  }

};



#endif //CC_SYNTHESIS_Z3INFERENCE_H
