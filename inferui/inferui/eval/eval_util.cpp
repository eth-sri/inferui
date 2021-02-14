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
#include "eval_util.h"
#include "eval_app_util.h"


DEFINE_string(train_data, "", "Training file with app data.");
DEFINE_string(test_data, "", "Testing file with app data.");


void adjustViewsByUserConstraints(App* syn_app) {
  Solver solver;
  App ref_rendered_app = JsonToApp(solver.sendPost(syn_app->ToJSON()));
  for (size_t i = 1; i < syn_app->GetViews().size(); i++) {
    View& ref_view = syn_app->GetViews()[i];
    View& ref_rendered_view = ref_rendered_app.GetViews()[i];

    ref_view.xleft = ref_rendered_view.xleft;
    ref_view.xright = ref_rendered_view.xright;
    ref_view.ytop = ref_rendered_view.ytop;
    ref_view.ybottom = ref_rendered_view.ybottom;
  }

}

std::pair<bool, std::vector<App>> applyTransformations(Solver& solver, const App& app, const std::vector<App> apps, std::string model, const Device ref_device, const std::vector<Device>& devices, std::string dataset, int& lowerLimit, int& upperLimit, int& necessaryUserCorrectionsSmaller, int& necessaryUserCorrectionsBigger){

	std::vector<App> transformedApps;
	Json::Value res(Json::objectValue);
	res["model"] = model;
	res["dataset"] = dataset;
	res["user_corrects"] = FLAGS_user_corrects;


	res["original"] = app.ToCoordinatesJSON();
	Json::Value ref_device_json(Json::arrayValue);
	ref_device_json.append(ref_device.width);
	ref_device_json.append(ref_device.height);
	res["ref_device"] = ref_device_json;

	if(apps.size() != devices.size()){
		LOG(FATAL) << "Sizes do not match in applyTransformations";
	}

	Json::Value devices_json(Json::arrayValue);
	for(auto device: devices){
		Json::Value device_json(Json::arrayValue);
		device_json.append(device.width);
		device_json.append(device.height);
		devices_json.append(device_json);
	}
	res["devices"] = devices_json;
	res["smaller_app"] = apps[0].ToCoordinatesJSON();;
	res["bigger_app"] = apps[1].ToCoordinatesJSON();;

	const Json::Value& result = solver.sendPostToTransformator(res);
	transformedApps.push_back(App(result["smaller_app"]));
	transformedApps.push_back(App(result["bigger_app"]));

	lowerLimit += result["realCandSmall"].asInt();
	upperLimit += result["realCandBig"].asInt();

	necessaryUserCorrectionsSmaller +=  result["necessaryUserCorrectionsSmaller"].asInt();
	necessaryUserCorrectionsBigger += result["necessaryUserCorrectionsBigger"].asInt();

	return std::make_pair(result["successful"].asBool(), transformedApps);
}

std::map<std::string, bool> CheckProperties(App ref_app, const Device& ref_device, const std::vector<Device>& devices) {
  std::map<std::string, bool> results;

  LayoutSolver layout_solver;
  Solver solver;
  LOG(INFO) << "Check Properties";
//  LOG(INFO) << ref_app.ToXml();
  LOG(INFO) << "render positions:";
  PrintApp(ref_app, true);

  const View& root = ref_app.GetViews()[0];
  for (const auto& device : devices) {
    App resized_app = ref_app;
//    View content_frame(root.xleft, root.ytop, root.xright, root.ybottom, root.name, root.id);
    View& content_frame = resized_app.GetViews()[0];
    TryResizeView(ref_app, content_frame, ref_device, device);
    LOG(INFO) << "ScreenResized: " << device;
    LOG(INFO) << '\t' << root;
    LOG(INFO) << '\t' << content_frame;


    std::pair<Status, App> layout_device_app = layout_solver.Layout(ref_app,
                                                                    content_frame.xleft, content_frame.ytop,
                                                                    content_frame.xright, content_frame.ybottom);


    if (layout_device_app.first != Status::SUCCESS) {
      LOG(INFO) << "Not Pixel Perfect";
      results["pixel_perfect"] = false;
//        continue;
    }

//    PrintApp(layout_device_app.second, false);
    Json::Value layout = solver.sendPost(resized_app.ToJSON());
    App syn_app = JsonToApp(layout);
    LOG(INFO) << "Resized App";
    PrintApp(syn_app, false);

    if (layout_device_app.first == Status::SUCCESS) {
      LOG(INFO) << "Layout App";
      PrintApp(layout_device_app.second, false);
//      CHECK(AppMatch(layout_device_app.second, layout));
    }

    if (!AppProperties::CheckBounds(syn_app)) {
      LOG(INFO) << "Check Bounds False";
      results["bounds"] = false;
    }

    if (!AppProperties::CheckIntersection(ref_app, syn_app)) {
      LOG(INFO) << "CheckIntersection False";
      results["intersection"] = false;
    }

    if (!AppProperties::CheckCentering(ref_app, syn_app)) {
      LOG(INFO) << "CheckCentering False";
      results["centering"] = false;
    }

    if (!AppProperties::CheckMargins(ref_app, syn_app)) {
      LOG(INFO) << "CheckMargins False";
      results["margins"] = false;
    }

    if (!AppProperties::CheckSizeRatio(ref_app, syn_app)) {
      LOG(INFO) << "CheckSizeRatio False";
      results["ratio"] = false;
    }
  }

//  if (!Contains(results, "pixel_perfect")) {
//    results["pixel_perfect"] = true;
  results.insert({"pixel_perfect", FindWithDefault(results, "pixel_perfect", true)});
  results.insert({"bounds", FindWithDefault(results, "bounds", true)});
  results.insert({"intersection", FindWithDefault(results, "intersection", true)});
  results.insert({"centering", FindWithDefault(results, "centering", true)});
  results.insert({"margins", FindWithDefault(results, "margins", true)});
  results.insert({"ratio", FindWithDefault(results, "ratio", true)});
//  }

  for (const auto& it : results) {
    LOG(INFO) << "\t\t" << it.first << ": " << it.second;
  }
  return results;
};
