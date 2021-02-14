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

#include <glog/logging.h>
#include "util.h"
#include "base/maputil.h"
#include "base/strutil.h"
#include "base/stringprintf.h"

std::string OrientationStr(const Orientation& cmd) {
  switch (cmd) {
    case Orientation::HORIZONTAL: return "Horizontal";
    case Orientation::VERTICAL: return "Vertical";
    default:
      LOG(FATAL) << "Unknown orientation";
  }
}

std::string ViewSizeStr(const ViewSize& size) {
  switch (size) {
    case ViewSize::FIXED: return "FIXED";
    case ViewSize::MATCH_CONSTRAINT: return "MATCH_CONSTRAINT";
    case ViewSize::MATCH_PARENT: return "MATCH_PARENT";
    default:
      LOG(FATAL) << "Unknown ViewSize";
  }
}

std::string ViewSizeStr(const ViewSize& size, int value) {
  if (size == ViewSize::MATCH_CONSTRAINT) {
    return Constants::name(Constants::MATCH_CONSTRAINT, Constants::OUTPUT_XML);
  } else if (size == ViewSize::MATCH_PARENT) {
    return Constants::name(Constants::MATCH_PARENT, Constants::OUTPUT_XML);
  } else {
    CHECK_EQ(size, ViewSize::FIXED);
  }

//  CHECK_EQ(value % 2, 0);
//  CHECK_GT(value / 2, 0);
  return StringPrintf("%dpx", value);
}

ViewSize GetViewSize(const ProtoView& view, Orientation orientation) {
  Constants::Name type = (orientation == Orientation::HORIZONTAL) ? Constants::layout_width : Constants::layout_height;
  CHECK(Contains(view.properties(), Constants::name(type)));
  std::string value = view.properties().at(Constants::name(type));
//  LOG(INFO) << value;
  return GetViewSize(value);
}

ViewSize GetViewSize(const std::string& value) {
  if (value == "match_parent" || value == "fill_parent") {
    return ViewSize::MATCH_PARENT;
  }
  if (ValueParser::hasValue(value) && ValueParser::parseValue(value) == 0) {
    return ViewSize::MATCH_CONSTRAINT;
  }
  //we dont differentiate between wrap_content and fixed
  return ViewSize::FIXED;
}

bool ViewsInsideScreen(const ProtoScreen& app) {
  const ProtoView& root = app.views(0);
  for (const ProtoView& view : app.views()) {
//    if (view.xleft() < 0 || view.xright() > app.device_width() ||
//        view.ytop() < 0 || view.ybottom() > app.device_height()) {
      //|| view.ybottom() > 1280
    if (view.xleft() < root.xleft() || view.xright() > root.xright() ||
        view.ytop() < root.ytop() || view.ybottom() > root.ybottom()) {
      return false;
    }
  }
  return true;
}

bool ValidNumberOfConstraints(const ProtoView& view) {
  if (view.type() == Constants::name(Constants::Guideline)) return true;

  if (CountProperties(view, Properties::LEFT_CONSTRAINT_LAYOUT_CONSTRAINTS) > 1 ||
      CountProperties(view, Properties::RIGHT_CONSTRAINT_LAYOUT_CONSTRAINTS) > 1 ||
      CountProperties(view, Properties::LEFT_CONSTRAINT_LAYOUT_CONSTRAINTS) + CountProperties(view, Properties::RIGHT_CONSTRAINT_LAYOUT_CONSTRAINTS) == 0 ||
      CountProperties(view, Properties::TOP_CONSTRAINT_LAYOUT_CONSTRAINTS) > 1 ||
      CountProperties(view, Properties::BOTTOM_CONSTRAINT_LAYOUT_CONSTRAINTS) > 1 ||
      CountProperties(view, Properties::TOP_CONSTRAINT_LAYOUT_CONSTRAINTS) + CountProperties(view, Properties::BOTTOM_CONSTRAINT_LAYOUT_CONSTRAINTS) == 0) {
//    LOG(INFO) << "Invalid View: " << view.DebugString();
    return false;
  }
  return true;
}

bool ValidConstraints(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    bool ignored = view.parent_seq_id() == -1 || app.views(view.parent_seq_id()).type() != Constants::name(Constants::ConstraintLayout);
    if (!ignored && !ValidNumberOfConstraints(view)) {

//      LOG(INFO) << "View " << view.seq_id() << " invalid";
//      LOG(INFO) << view.DebugString();
      return false;
    }
  }
  return true;
}

bool ResolvedConstraints(const ProtoScreen& app) {
  ConstraintStats stats = ForEachConstraint(
      app,
      Constants::ConstraintLayout,
      [](const Constants::Name& name, const ProtoView& src, const ProtoView& tgt){}
  );
  return stats.unresolved == 0;
}

bool HasConstraintBias(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    if (Contains(view.properties(), Constants::name(Constants::layout_constraintHorizontal_bias)) ||
        Contains(view.properties(), Constants::name(Constants::layout_constraintVertical_bias))) {
      return true;
    }
  }
  return false;
}

bool HasBaselineConstraint(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    if (Contains(view.properties(), Constants::name(Constants::layout_constraintBaseline_toBaselineOf))) {
      return true;
    }
  }
  return false;
}

bool HasChainConstraint(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    if (Contains(view.properties(), Constants::name(Constants::layout_constraintHorizontal_chainStyle)) ||
        Contains(view.properties(), Constants::name(Constants::layout_constraintVertical_chainStyle))) {
      return true;
    }
  }
  return false;
}

bool HasGuideline(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    if (view.type() == Constants::name(Constants::Guideline)) {
      return true;
    }
  }
  return false;
}

bool NegativeViewSize(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    if (view.xright() <= view.xleft() || view.ybottom() <= view.ytop()) {
      return true;
    }
  }
  return false;
}

bool ValidMargins(const ProtoScreen& app) {
  for (const ProtoView& view : app.views()) {
    if (GetMarginFromProto(view, Constants::layout_marginLeft) < 0) return false;
    if (GetMarginFromProto(view, Constants::layout_marginRight) < 0) return false;
    if (GetMarginFromProto(view, Constants::layout_marginTop) < 0) return false;
    if (GetMarginFromProto(view, Constants::layout_marginBottom) < 0) return false;
  }
  return true;
}


//bool HasDepreceatedAttrs(const ProtoScreen& app) {
//  for (const ProtoView& view : app.views()) {
//    if (view.seq_id() == 0 || InRootConstraintLayout(app, view)) continue;
//    if (GetViewSize(view, Orientation::HORIZONTAL) == ViewSize::MATCH_PARENT ||
//        GetViewSize(view, Orientation::VERTICAL) == ViewSize::MATCH_PARENT) {
//      return true;
//    }
//  }
//  return false;
//}

//bool UnsupportedConstraints(const ProtoScreen& app) {
//  return HasBaselineConstraint(app);
//}

bool ValidApp(const ProtoScreen& app, ValueCounter<std::string>* stats) {
  if (!ViewsInsideScreen(app)) {
    if (stats != nullptr) {
      stats->Add("views_outsideapp");
    }
    return false;
  }
  if (HasBaselineConstraint(app)) {
    if (stats != nullptr) {
      stats->Add("baseline_constraint");
    }
    return false;
  }
  if (!ValidConstraints(app)) {
    if (stats != nullptr) {
      stats->Add("invalid_constraint");
    }
    return false;
  }
  if (!ResolvedConstraints(app)) {
    if (stats != nullptr) {
      stats->Add("unresolved_constraint");
    }
    return false;
  }
  if (HasGuideline(app)) {
    if (stats != nullptr) {
      stats->Add("has_guideline");
    }
    return false;
  }
  if (HasChainConstraint(app)) {
    if (stats != nullptr) {
      stats->Add("has_chain_constraints");
    }
    return false;
  }
  if (!ValidMargins(app)) {
    if (stats != nullptr) {
      stats->Add("invalid_margins");
    }
    return false;
  }
  if (NegativeViewSize(app)) {
    if (stats != nullptr) {
      stats->Add("negative_view_size");
    }
    return false;
  }
//  if (HasDepreceatedAttrs(app)) {
//    if (stats != nullptr) {
//      stats->Add("has_depreceated_attrs");
//    }
//    return false;
//  }
  if (stats != nullptr) stats->Add("ok");
  return true;
//  return (ViewsInsideScreen(app) && ValidConstraints(app) && ResolvedConstraints(app));
}



int CountProperties(const ProtoView& view, const std::vector<Constants::Name>& properties) {
  int count = 0;
  for (const Constants::Name &property : properties) {
    if (Contains(view.properties(), Constants::name(property))) {
      count++;
    }
  }
  return count;
}

bool HasRelativeConstraint(const ProtoView& view) {
  for (const Constants::Name &property : Properties::RELATIVE_CONSTRAINTS) {
    if (Contains(view.properties(), Constants::name(property))) {
      return true;
    }
  }
  return false;
}

bool HasRelativeParentConstraint(const ProtoView& view) {
  for (const Constants::Name &property : Properties::RELATIVE_PARENT_CONSTRAINTS) {
    if (Contains(view.properties(), Constants::name(property))) {
      return true;
    }
  }
  return false;
}

bool InRootConstraintLayout(const ProtoScreen& app, const ProtoView& view) {
  return InRootLayout(app, view, Constants::ConstraintLayout);
}

bool InRootLayout(const ProtoScreen& app, const ProtoView& view, Constants::Name layout_type) {
  return view.parent_seq_id() == -1 ||
      (InRootLayout(app, app.views(view.parent_seq_id()), layout_type) &&
          app.views(view.parent_seq_id()).type() == Constants::name(layout_type)
      );
}

void PrintLayoutInner(const ProtoScreen& app, const ProtoView& view, std::stringstream& ss, int indent = 0) {
  ss << "\n";
  for (int i = 0; i < indent; i++) {
    ss << '\t';
  }
  if (InRootConstraintLayout(app, view)) {
    ss << "*";
  }
  ss << view.type().c_str();
  for (int child : view.children_ids()) {
    PrintLayoutInner(app, app.views(child), ss, indent + 1);
  }
}

std::string PrintLayout(const ProtoScreen& app) {
  std::stringstream ss;
  PrintLayoutInner(app, app.views(0), ss, 0);
  return ss.str();
}



std::unordered_set<std::string> GetReferencedIds(const ProtoScreen& app) {
  std::unordered_set<std::string> res;
  for (const ProtoView& view : app.views()) {
    for (const Constants::Name &property_name : Properties::RELATIVE_CONSTRAINTS) {
      const std::string& property = Constants::name(property_name);
      if (Contains(view.properties(), property)) {
        res.insert(view.properties().at(property));
      }
    }
  }
  return res;
}



const ProtoView* FindViewById(const ProtoScreen& app, const ProtoView& view, const std::string& id) {
  if (id == "parent") {
    if (view.parent_seq_id() == -1) return nullptr;
    return &app.views(view.parent_seq_id());
  }
  std::string normalized_id = ValueParser::parseID(id);
//  CHECK(!normalized_id.empty()) << id << "\n" << app.DebugString();
  if (normalized_id.empty()) {
    LOG(INFO) << "invalid id: " << id;
    return nullptr;
  }
  for (const ProtoView& view : app.views()) {
    if (ValueParser::parseID(view.id()) == normalized_id) return &view;
  }
  std::vector<std::string> ids;
  for (const ProtoView& view : app.views()) {
    ids.push_back(view.id());
  }
//  LOG(INFO) << "View with id '" << id << "' not found in: \n\t" << JoinStrings(ids, ", ");//:\n" << app.DebugString();
  return nullptr;
}

const ProtoView* FindPropertyTarget(const ProtoScreen& app, const ProtoView& view, const Constants::Name& property_name) {
  const std::string& property = Constants::name(property_name);
  if (!Contains(view.properties(), property)) return nullptr;

  return FindViewById(app, view, view.properties().at(property));
}

std::pair<const ProtoView*, Constants::Name> FindPropertyTarget(const ProtoScreen& app, const ProtoView& view, const std::vector<Constants::Name>& properties) {
  for (const auto& property : properties) {
    const ProtoView* tgt = FindPropertyTarget(app, view, property);
    if (tgt != nullptr) return std::make_pair(tgt, property);
  }
  return std::make_pair<const ProtoView*, Constants::Name>(nullptr, Constants::NO_NAME);
}


ConstraintStats ForEachConstraint(const ProtoScreen& app, Constants::Name type, const std::function<void(const Constants::Name&, const ProtoView&, const ProtoView&)>& cb) {
  ConstraintStats stats;
  for (const ProtoView& view : app.views()) {
    // The Properties are enabled only if the parent view is of given type
    if (view.parent_seq_id() == -1) continue;
    bool ignored = !InRootLayout(app, view, type);//view.parent_seq_id() == -1 || app.views(view.parent_seq_id()).type() != Constants::name(type);

    for (const Constants::Name &property_name : Constants::ViewProperties(type)) {
      const std::string& property = Constants::name(property_name);
      if (!Contains(view.properties(), property)) continue;

      stats.total++;
      if (ignored) {
        stats.ignored++;
        continue;
      }
      const ProtoView* ref_view = FindViewById(app, view, view.properties().at(property));
      if (ref_view == nullptr) {// || !InRootLayout(app, *ref_view, type)) {
        stats.unresolved++;
        continue;
      }

      cb(property_name, view, *ref_view);
    }
  }
  return stats;
}

void ForEachConstraintLayoutConstraint(
    const ProtoScreen& app,
    const std::function<void(const ProtoView&, std::pair<const ProtoView*, Constants::Name>&, std::pair<const ProtoView*, Constants::Name>&, const Orientation&)>& cb) {
  for (const ProtoView& view : app.views()) {
    if (view.parent_seq_id() == -1) continue;
    bool ignored = !InRootLayout(app, view, Constants::ConstraintLayout);
//    bool ignored = view.parent_seq_id() == -1 || app.views(view.parent_seq_id()).type() != Constants::name(Constants::ConstraintLayout);

    if (ignored) {
      continue;
    }

    if (view.type() == Constants::name(Constants::Guideline)) {
      continue;
    }

//    LOG(INFO) << view.DebugString();

    std::pair<const ProtoView*, Constants::Name> left = FindPropertyTarget(app, view, Properties::LEFT_CONSTRAINT_LAYOUT_CONSTRAINTS);
    std::pair<const ProtoView*, Constants::Name> right = FindPropertyTarget(app, view, Properties::RIGHT_CONSTRAINT_LAYOUT_CONSTRAINTS);

    CHECK(left.first != nullptr || right.first != nullptr);
    cb(view, left, right, Orientation::HORIZONTAL);

    std::pair<const ProtoView*, Constants::Name> top = FindPropertyTarget(app, view, Properties::TOP_CONSTRAINT_LAYOUT_CONSTRAINTS);
    std::pair<const ProtoView*, Constants::Name> bottom = FindPropertyTarget(app, view, Properties::BOTTOM_CONSTRAINT_LAYOUT_CONSTRAINTS);

    CHECK(top.first != nullptr || bottom.first != nullptr) << "View id: << " << view.seq_id() << "\n" << app.DebugString();
    cb(view, top, bottom, Orientation::VERTICAL);
  }
}

const std::vector<std::regex> ValueParser::value_regex = {
    std::regex("(-?\\d+\\.?\\d*)[sd]i?p"),
    std::regex("(\\d+)_?[sd]p"),
    std::regex("[sd]p_?(\\d+)"),
    std::regex("^(\\d+)$"),
};

const std::regex ValueParser::id_regex = std::regex("\"?@\\+?(.*)\"?");
const std::regex ValueParser::view_id_regex = std::regex("@\\+id/view([0-9]+)");
const std::regex ValueParser::px_regex = std::regex("^(\\d+)px$");

bool ValueParser::hasValue(const std::string& input) {
  for (const auto& regex : value_regex) {
    std::smatch match;
    if(std::regex_search(input, match, regex)) {
      return true;
    }
  }
  return false;
}

float ValueParser::parseValue(const std::string& input) {
  for (const auto& regex : value_regex) {
    std::smatch match;
    if(std::regex_search(input, match, regex)) {
      float res;
      ParseFloat(match[1], &res);
      return res;
    }
  }
  LOG(INFO) << "no value found in " << input << ". Ensure that hasValue is called before calling parseValue.";
  return 0;
}

bool ValueParser::hasPxValue(const std::string& input) {
  std::smatch match;
  return std::regex_search(input, match, px_regex);
}

int ValueParser::parsePxValue(const std::string& input) {
  std::smatch match;
  if(std::regex_search(input, match, px_regex)) {
    return ParseInt32(match[1]);
  }
  return 0;
}

std::string ValueParser::parseID(const std::string& input) {
  std::smatch match;
  if(std::regex_search(input, match, id_regex)) {
    return match[1];
  }
  return "";
}

int ValueParser::parseViewSeqID(const std::string& input) {
  std::smatch match;
  if(std::regex_search(input, match, view_id_regex)) {
    return ParseInt32(match[1]);
  }
  LOG(FATAL) << "unknown input: " << input;
}

void ConstraintStats::Merge(const ConstraintStats& other) {
  total += other.total;
  unresolved += other.unresolved;
  ignored += other.ignored;

  total_apps++;
  if (other.unresolved > 0) unresolved_apps++;
  if (other.ignored > 0) ignored_apps++;
}

std::string ConstraintStats::ToString() const {
  std::stringstream ss;

  ss << "\n\tTotal Constraints: " << total
     << ", Unresolved: " << unresolved << " (" << (unresolved * 100.0 / total) << "%)"
     << ", Ignored: " << ignored << " (" << (ignored * 100.0 / total) << "%)";

  ss << "\n\tTotal Apps: " << total_apps
     << ", Unresolved: " << unresolved_apps << " (" << (unresolved_apps * 100.0 / total_apps) << "%)"
     << ", Ignored: " << ignored_apps << " (" << (ignored_apps * 100.0 / total_apps) << "%)";

  return ss.str();
}

float GetMarginFromProto(const ProtoView& proto_view, const Constants::Name& type) {
  if (type == Constants::NO_NAME) return 0;
  if (Contains(proto_view.properties(), Constants::name(type))) {
    const std::string& value = proto_view.properties().at(Constants::name(type));
//    LOG(INFO) << value;
    if (ValueParser::hasValue(value)) {
//      LOG(INFO) << ValueParser::parseValue(value);
      return ValueParser::parseValue(value) * 2;
    }

    return 0;
  } else if (Contains(proto_view.properties(), Constants::name(Constants::layout_margin))){
    const std::string& value = proto_view.properties().at(Constants::name(Constants::layout_margin));
    if (ValueParser::hasValue(value)) {
      return ValueParser::parseValue(value) * 2;
    }
  }
  return 0;
}

float GetBiasFromProto(const ProtoView& proto_view, const Orientation& orientation) {
  Constants::Name type = (orientation == Orientation::HORIZONTAL) ? Constants::layout_constraintHorizontal_bias
                                                                  : Constants::layout_constraintVertical_bias;

  if (Contains(proto_view.properties(), Constants::name(type))) {
    const std::string& value = proto_view.properties().at(Constants::name(type));
    float bias;
    CHECK(ParseFloat(value, &bias));
    return bias;
  }
  return 0.5;
}

void Padding::Initialize(const ProtoView& view) {

  //paddingLeft

  if (Contains(view.properties(), Constants::name(Constants::paddingLeft))) {
    paddingLeft = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingLeft)));
  }

  if (Contains(view.properties(), Constants::name(Constants::paddingRight))) {
    paddingRight = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingRight)));
  }

  if (Contains(view.properties(), Constants::name(Constants::paddingTop))) {
    paddingTop = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingTop)));
  }

  if (Contains(view.properties(), Constants::name(Constants::paddingBottom))) {
    paddingBottom = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingBottom)));
  }


  //paddingHorizontal
  if (Contains(view.properties(), Constants::name(Constants::paddingHorizontal))) {
    paddingLeft = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingHorizontal)));
    paddingRight = paddingLeft;
  }

  if (Contains(view.properties(), Constants::name(Constants::paddingVertical))) {
    paddingTop = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingVertical)));
    paddingBottom = paddingTop;
  }

  //padding
  if (Contains(view.properties(), Constants::name(Constants::padding))) {
    paddingLeft = ValueParser::parseValue(view.properties().at(Constants::name(Constants::padding)));
    paddingRight = paddingLeft;
    paddingTop = paddingLeft;
    paddingBottom = paddingLeft;
  }

  //paddingStart
  if (Contains(view.properties(), Constants::name(Constants::paddingStart))) {
    paddingLeft = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingStart)));
  }
  if (Contains(view.properties(), Constants::name(Constants::paddingEnd))) {
    paddingRight = ValueParser::parseValue(view.properties().at(Constants::name(Constants::paddingEnd)));
  }


  paddingLeft *= 2;
  paddingRight *= 2;
  paddingTop *= 2;
  paddingBottom *= 2;
}