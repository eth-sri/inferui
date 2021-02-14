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
#include "util/recordio/recordio.h"

#include "inferui/model/uidump.pb.h"
#include "base/maputil.h"
#include "inferui/model/model.h"
#include "inferui/model/constraints.h"
#include "base/fileutil.h"
#include "inferui/model/synthesis.h"
#include "base/iterutil.h"
#include "inferui/layout_solver/solver.h"
#include "json/json.h"
#include "inferui/model/util/util.h"
#include "inferui/model/constraint_model.h"

DEFINE_string(data, "uidumps.proto", "File with app data.");

DEFINE_string(model, "attr.model", "File with serialized models.");

float GetMargin(const ProtoView& proto_view, const Constants::Name& type) {
  if (type == Constants::NO_NAME) return 0;
  if (Contains(proto_view.properties(), Constants::name(type))) {
    const std::string& value = proto_view.properties().at(Constants::name(type));
//    LOG(INFO) << value;
    if (ValueParser::hasValue(value)) {
//      LOG(INFO) << ValueParser::parseValue(value);
      return ValueParser::parseValue(value);
    }

    return 0;
  }
  return 0;
}

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);


//  std::vector<Feature> features;
//  features.emplace_back(Feature("distance", Features::GetDistance));
//  features.emplace_back(Feature("intersections", Features::NumIntersections));
//  features.emplace_back(Feature("type", Features::GetType));
//  features.emplace_back(Feature("angle", Features::GetAngle));
//  features.emplace_back(Feature("margin", Features::GetMargin));

//  App syn_app;
//  syn_app.views.push_back(View(0, 0, 720, 1080, "0", 0));
//  syn_app.views.push_back(View(16, 16, 688, 106, "1", 1));
//  syn_app.views.push_back(View(16, 138, 688, 1030, "2", 2));
//  syn_app.views.push_back(View(16, 138, 688, 1030, "3", 3));
//
//
//
//
//  Attribute attr(ConstraintType::L2LxR2R, 0, &syn_app.views[1], &syn_app.views[0], &syn_app.views[0]);
//  LOG(INFO) << attr;
//  for (const Feature& f : features) {
//    std::pair<float, float> value = f.ValueAttr(attr, syn_app.views);
//    LOG(INFO) << "\t" << f.name << ": " << value.first << ", " << value.second;
//  }
//
//  return 0;

//  CountingFeatureModel marginModel = GetMarginModel();
  ConstraintModelWrapper model_wrapper;
  model_wrapper.Train(FLAGS_data);
//  int invalid_apps = 0;
//  int app_id = 0;
//  ForEachRecord<ProtoApp>(FLAGS_data.c_str(), [&](const ProtoApp &app) {
//    app_id++;
//    CHECK_GT(app.screens_size(), 0);
//    const ProtoScreen& screen = app.screens(0);
//    if (!ValidApp(screen)) {
//      invalid_apps++;
//      return true;
//    }
//
//
//    App syn_app(screen);
////    for (auto& view : syn_app.views) {
////      LOG(INFO) << view.id << ": location["
////                << view.xleft << ", "
////                << view.ytop << ", "
////                << (view.xright - view.xleft) << ", "
////                << (view.ybottom - view.ytop) << "]";
////    }
//
//    ForEachConstraintLayoutConstraint(screen, [&](
//        const ProtoView& src,
//        std::pair<const ProtoView*, Constants::Name>& start,
//        std::pair<const ProtoView*, Constants::Name>& end){
////      LOG(INFO) << src.DebugString();
////      LOG(INFO) << "\t" << Constants::name(start.second) << ", " << Constants::name(end.second);
//
//        Attribute attr(&syn_app.views[src.seq_id()],
//                       GetMargin(src, ConstraintTypeToMargin(start.second)), GetMargin(src, ConstraintTypeToMargin(end.second)),
//                       (start.first != nullptr) ? &syn_app.views[start.first->seq_id()] : nullptr, start.second,
//                       (end.first != nullptr) ? &syn_app.views[end.first->seq_id()] : nullptr, end.second);
//      model_wrapper.AddAttr(attr, syn_app.views);
////
////      LOG(INFO) << "==========";
////      model_wrapper.Dump();
////      LOG(INFO) << model_wrapper;
////      LOG(INFO) << attr;
////
////      for (const Feature& f : features) {
////        std::pair<float, float> value = f.ValueAttr(attr, syn_app.views);
////        LOG(INFO) << "\t" << f.name << ": " << value.first << ", " << value.second;
////        LOG(INFO) << value;
////      }
//    });
//
//    return true;
//  });

  model_wrapper.Dump();

  FILE *file = fopen(FLAGS_model.c_str(), "wb");
  model_wrapper.SaveOrDie(file);
  fclose(file);
//  LOG(INFO) << "Num apps: " << app_id << ", Invalid apps: " << invalid_apps;


  return 0;
}
