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

#include "syn_helper.h"
#include "base/fileutil.h"
#include "base/range.h"

#include <unordered_set>

/*
 *
 */

std::vector<Json::Value> JsonAppSerializer::readDirectory(const std::string &path) {
  std::vector<Json::Value> json_apps;
  std::vector <std::string> files = FindFiles(path.c_str(), ".txt");
  std::sort(files.begin(), files.end());

  Json::Reader reader;
  for (std::string filename : files) {
    std::ifstream input(filename);
    CHECK(input);
    Json::Value root;
    reader.parse(input, root);
    root["id"] = filename;
    json_apps.push_back(root);
  }

  return json_apps;
}

std::vector<Json::Value> JsonAppSerializer::readFile(const std::string& path) {
  std::string content = ReadFileToStringOrDie(path.c_str());
  std::vector<std::string> lines;
  SplitStringUsing(content, '\n', &lines, false);

  std::vector<Json::Value> json_apps;
  Json::Reader reader;
  int i = 0;
  for (const std::string& line : lines) {
    Json::Value root;
    reader.parse(line, root);
    //root["id"] = i++;
    json_apps.push_back(root);
  }

  return json_apps;
}

Json::Value DeviceToJson(const Device& device) {
  Json::Value resolution(Json::arrayValue);
  resolution.append(device.width);
  resolution.append(device.height);
  return resolution;
}

Json::Value ViewToJson(const View& view) {
  Json::Value res(Json::arrayValue);
  res.append(view.xleft);
  res.append(view.ytop);
  res.append(view.xright);
  res.append(view.ybottom);
  return res;
}

Json::Value AppToJson(const App& app) {
  Json::Value res(Json::arrayValue);
  for (const View& view : app.GetViews()) {
    if (view.is_content_frame()) continue;
    res.append(ViewToJson(view));
  }
  return res;
}

Json::Value JsonAppSerializer::ScreenToJson(const App& app, const Device& device) {
  Json::Value screen(Json::objectValue);
  screen["resolution"] = DeviceToJson(device);
  screen["views"] = AppToJson(app);
  return screen;
}

//  {"screens":
//    [
//      {"resolution": [350, 630], "views": [[15, 168, 335, 413], [18, 177, 334, 240]]},
//      {"resolution": [360, 640], "views": [[20, 173, 340, 418], [23, 182, 339, 245]]}
//    ]
//  }
Json::Value JsonAppSerializer::ToJson(const App& app, const std::vector<App>& apps,
                          const Device& ref_device, const std::vector<Device>& devices) {
  CHECK_EQ(devices.size(), apps.size());
  Json::Value root(Json::objectValue);

  Json::Value screens(Json::arrayValue);
  screens.append(JsonAppSerializer::ScreenToJson(app, ref_device));
  for (size_t i = 0; i < devices.size(); i++) {
    screens.append(JsonAppSerializer::ScreenToJson(apps[i], devices[i]));
  }
  root["screens"] = screens;
  return root;
}

void JsonAppSerializer::AddScreenToJson(const App& app, const Device& device, Json::Value* data) {
  if (!data->isMember("screens")) {
    (*data)["screens"] = Json::Value(Json::arrayValue);
  }

  (*data)["screens"].append(JsonAppSerializer::ScreenToJson(app, device));
}

/*
 * From eval_app_util, remove after merge
 *
 */

App CuJsonToAppRaw(const Json::Value& screen) {
  App app;
  app.AddView(View(0, 0, screen["resolution"][0].asInt(), screen["resolution"][1].asInt(), "parent", 0, "parent"));
  int i = 0;
  for (auto view : screen["views"]) {
    if(view[0].asInt() == 0 &&  view[1].asInt() == 0 && view[2].asInt() == screen["resolution"][0].asInt() && view[3].asInt() == screen["resolution"][1].asInt()){
      continue;
    }

    //view coordinates are added in this order [view.xleft, view.ytop, view.xright, view.ybottom] in oracle_synthesizer.py
    i++;
    app.AddView(View(view[0].asInt(), view[1].asInt(), view[2].asInt(), view[3].asInt(), "frog", i, "frog"));
  }
  //just assume that views are resizable, if no layout should be generated as well
  std::vector<bool> resizable(2, true);
  app.setResizable(resizable);
  return app;
}

int JsonAppSerializer::JsonToApps(const Json::Value& request, App& app, std::vector<App>& apps, Device& ref_device, std::vector<Device>& devices){
  // TODO: the dataset should already contain them in the order they are supposed to be, here the it should only read the data
  int idx = ParseInt32(request.get("id", "-1").asString());
  //if there are three -> reorder that it take the middle one as ref device
  if(request["screens"].size() == 3){
    auto layoutSmall = request["screens"][0];
    auto layoutMiddle = request["screens"][1];
    auto layoutBig = request["screens"][2];

    bool firstOneSmaller = layoutSmall["resolution"][0].asInt() < layoutMiddle["resolution"][0].asInt();
    bool secondOneSmaller = layoutMiddle["resolution"][0].asInt() < layoutBig["resolution"][0].asInt();

    //make sure that they are ordered as expected
    if(!(firstOneSmaller && secondOneSmaller)){
      LOG(FATAL) << "device sizes are not ordered as expected";
    }

    app = CuJsonToAppRaw(layoutMiddle);
    ref_device = Device(layoutMiddle["resolution"][0].asInt(), layoutMiddle["resolution"][1].asInt());

    devices.push_back(Device(layoutSmall["resolution"][0].asInt(), layoutSmall["resolution"][1].asInt()));
    devices.push_back(Device(layoutBig["resolution"][0].asInt(), layoutBig["resolution"][1].asInt()));
    apps.push_back(CuJsonToAppRaw(layoutSmall));
    apps.push_back(CuJsonToAppRaw(layoutBig));
  }else{
    int i = 0;
    for (const auto& screen : request["screens"]) {
      //take the first as reference app
      if(i == 0){
        app = CuJsonToAppRaw(screen);
        ref_device = Device(screen["resolution"][0].asInt(), screen["resolution"][1].asInt());
      }else{
        devices.push_back(Device(screen["resolution"][0].asInt(), screen["resolution"][1].asInt()));
        //add the next one in the app vector
        const App& app = CuJsonToAppRaw(screen);
        apps.push_back(app);
      }
      i = i + 1;
    }
  }
  return idx;
}

/*
 *
 */


std::vector<int> FindNonMatchingViews(const App& ref_app, const App& syn_app, const Orientation& orientation) {
  CHECK_EQ(ref_app.GetViews().size(), syn_app.GetViews().size());
  std::vector<int> view_ids;
  for (size_t i = 0; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if ((orientation == Orientation::HORIZONTAL && (ref_view.xleft != syn_view.xleft || ref_view.xright != syn_view.xright)) ||
        (orientation == Orientation::VERTICAL && (ref_view.ytop != syn_view.ytop || ref_view.ybottom != syn_view.ybottom))) {
      view_ids.push_back(ref_view.id);
    }
  }
  return view_ids;
}

bool IsIndependent(const View& node, const Orientation& orientation, const std::vector<int> ids) {
  std::unordered_set<int> nodes = node.ReferencedNodes(orientation);

  std::vector<int> referenced_nodes(nodes.begin(), nodes.end());
  std::sort(referenced_nodes.begin(), referenced_nodes.end());

  std::vector<int> v_intersection;
  std::set_intersection(ids.begin(), ids.end(),
                        referenced_nodes.begin(), referenced_nodes.end(),
                        std::back_inserter(v_intersection));

  // we check for == 1 as ReferencedNodes always includes the source node
  return v_intersection.size() == 1;
}

void TryFixNode(View& ref_view, const View& rendered_view, const Orientation& orientation) {
  int diff = (Orientation::VERTICAL == orientation) ? ref_view.ytop - rendered_view.ytop : ref_view.xleft - rendered_view.xleft;

  LOG(INFO) << "Changing: " << ref_view;
  Attribute& attr = ref_view.attributes.at(orientation);
  if (attr.value_primary == 0 && attr.value_secondary > 0) {

    LOG(INFO) << '\t' << attr;
    attr.value_secondary -= diff;
    LOG(INFO) << '\t' << attr;
  }
}

std::vector<int> FindUnnormalizedAttributes(const App& app, const Orientation& orientation, const std::unordered_set<int>& resolved_ids) {
  std::vector<int> view_ids;
  for (const View& view: app.GetViews()) {
//  for (size_t i = 0; i < app.GetViews().size(); i++) {
    if (Contains(resolved_ids, view.id)) continue;
//    const View& view = app.GetViews()[i];
    if (view.is_content_frame()) continue;
    const auto& attr = view.GetAttribute(orientation);
    if ((attr.value_primary == 0 && attr.value_secondary == 1) ||
        (attr.value_primary == 1 && attr.value_secondary == 0)) {
      view_ids.push_back(view.id);
    }
  }
  return view_ids;
}

std::vector<int> FindUnnormalizedAttributes(const App& app, const Orientation& orientation) {
  std::unordered_set<int> resolved_ids;
  return FindUnnormalizedAttributes(app, orientation, resolved_ids);
}

bool HasUnnormalizedAttributes(const App& app) {
  return !FindUnnormalizedAttributes(app, Orientation::HORIZONTAL).empty() ||
         !FindUnnormalizedAttributes(app, Orientation::VERTICAL).empty();
}

void NormalizeMargins(App* ref_app, Solver& solver) {
  if (!HasUnnormalizedAttributes(*ref_app)) return;

  App app = *ref_app;
  App rendered_app = JsonToApp(solver.sendPost(app.ToJSON()));
  if (!AppMatch(app, rendered_app)) {
    return;
  }

  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    LOG(INFO) << "Orientation: " << orientation;
    std::unordered_set<int> resolved_ids;
    auto view_ids = FindUnnormalizedAttributes(app, orientation, resolved_ids);
    while (!view_ids.empty()) {
      LOG(INFO) << "Views " << JoinInts(view_ids, ',') << " do not match";
      for (int view_id : view_ids) {
        View &view = app.findView(view_id);

        if (!IsIndependent(view, orientation, view_ids)) {
          LOG(INFO) << "Skipping dependent: " << view_id;
          continue;
        }

        Attribute& attr = view.attributes.at(orientation);
        if ((attr.value_primary == 0 && attr.value_secondary == 1) ||
            (attr.value_primary == 1 && attr.value_secondary == 0)) {
          LOG(INFO) << "Changing: " << attr;
          attr.value_primary = 0;
          attr.value_secondary = 0;
        }
        resolved_ids.insert(view_id);
        break;
      }

      rendered_app = JsonToApp(solver.sendPost(app.ToJSON()));
      if (!AppMatch(app, rendered_app)) {
        LOG(INFO) << "Failed, Apps do not match";
        app = *ref_app;
      } else {
        LOG(INFO) << "Success";
        *ref_app = app;
      }
      view_ids = FindUnnormalizedAttributes(app, orientation, resolved_ids);
    }
  }

  return;
}

bool TryFixInconsistencies(App* ref_app, Solver& solver) {
  App app = *ref_app;
  App rendered_app = JsonToApp(solver.sendPost(app.ToJSON()));
  if (AppMatch(app, rendered_app)) return true;

  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    LOG(INFO) << "Orientation: " << orientation;
    auto view_ids = FindNonMatchingViews(app, rendered_app, orientation);
    while (!view_ids.empty()) {
      LOG(INFO) << "Views " << JoinInts(view_ids, ',') << " do not match";
      for (int view_id : view_ids) {
        View &ref_view = app.findView(view_id);
        const View &rendered_view = rendered_app.findView(view_id);

        if ((orientation == Orientation::VERTICAL && std::abs(ref_view.ytop - rendered_view.ytop) == 1) ||
            (orientation == Orientation::HORIZONTAL && std::abs(ref_view.xleft - rendered_view.xleft) == 1)) {
          if (!IsIndependent(ref_view, orientation, view_ids)) {
            LOG(INFO) << "Skipping dependent: " << view_id;
            continue;
          }
          TryFixNode(ref_view, rendered_view, orientation);
        }
      }

      rendered_app = JsonToApp(solver.sendPost(app.ToJSON()));
      auto cview_ids = FindNonMatchingViews(app, rendered_app, orientation);
      if (cview_ids.size() == view_ids.size()) {
        // fixing view did not help
        LOG(INFO) << "Could not fix all the views";
//        PrintApp(app);
//        PrintApp(rendered_app, false);
        return false;
      }
      view_ids = cview_ids;
    }
  }
//  LOG(INFO) << "Original";
//  PrintApp(*ref_app);
  *ref_app = app;
//  LOG(INFO) << "Fixed";
//  PrintApp(*ref_app);
  return true;
}

/*
 *
 */

App EmptyApp(const App& ref, const Device& device) {
  App app;
  for (const View& view : ref.GetViews()) {
    if (view.is_content_frame()) {
      app.AddView(View(view.xleft, view.ytop, view.xleft + device.width, view.ytop + device.height, view.name, view.id));
    } else {
      app.AddView(View(-1, -1, -1, -1, view.name, view.id));
    }
  }
  return app;
}

App EmptyApp(const App& ref) {
  App app;
  for (const View& view : ref.GetViews()) {
    if (view.is_content_frame()) {
      app.AddView(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
    } else {
      app.AddView(View(-1, -1, -1, -1, view.name, view.id));
    }
  }
  return app;
}

App KeepFirstNViews(const App& ref, int num_views) {
  App app;
  for (const auto& view : ref.GetViews()) {
    if (app.GetViews().size() >= num_views) break;

    app.AddView(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
  }
  return app;
}

bool ViewMatch(const View& A, const View& B) {
  return A.xleft == B.xleft && A.xright == B.xright &&
         A.ytop == B.ytop && A.ybottom == B.ybottom;
}

bool AppMatch(const App& ref_app, const App& syn_app) {
  if (ref_app.GetViews().size() != syn_app.GetViews().size()) return false;
  for (size_t i = 0; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if (!ViewMatch(ref_view, syn_view)) {
      return false;
    }
  }
  return true;
}

View JsonToView(const Json::Value& value) {
  return View(value["location"][0].asInt(), value["location"][1].asInt(),
              value["location"][2].asInt() + value["location"][0].asInt(),
              value["location"][3].asInt() + value["location"][1].asInt(),
              value["id"].asString(), !value.isMember("id") ? 0 : ValueParser::parseViewSeqID(value["id"].asString())
  );
}

App JsonToApp(const Json::Value& layout) {
  App app;
  app.AddView(JsonToView(layout["content_frame"]));
  for (auto component : layout["components"]) {
    app.AddView(JsonToView(component));
  }
  return app;
}

void ScaleAppInner(Json::Value& value, double factor) {
  if (value.isArray()) {
    for (auto& elem : value) {
      ScaleAppInner(elem, factor);
    }
  } else if (value.isObject()) {
    for( Json::Value::iterator it = value.begin() ; it != value.end() ; it++) {
      if (it->isString()) {
        if (ValueParser::hasPxValue(it->asString())) {

          *it = StringPrintf("%dpx", (int) (factor * ValueParser::parsePxValue(it->asString())));
        }
      } else {
        ScaleAppInner(*it, factor);
      }
    }
  }
}

Json::Value ScaleApp(Json::Value value, double factor) {
  if (factor == 1) return value;
  ScaleAppInner(value, factor);
  return value;
}

void ScaleAttributes(App& app, double scaling_factor) {
  for (View& view : app.GetViews()) {
    for (auto& it : view.attributes) {
      it.second.value_primary *= scaling_factor;
      it.second.value_secondary *= scaling_factor;
    }
    view.padding.paddingBottom *= scaling_factor;
    view.padding.paddingTop *= scaling_factor;
    view.padding.paddingLeft *= scaling_factor;
    view.padding.paddingRight *= scaling_factor;
  }
}

void eraseSubString(std::string &orginal, const std::string& toErase){
	// Search for the substring in string
	size_t pos = orginal.find(toErase);

	if (pos != std::string::npos)
	{
		// If found then erase it from string
		orginal.erase(pos, toErase.length());
	}
}

//returns score which oracle predicts
double askOracle(
		const App& candidate, //the candidate for which the probability should be predicted
		Solver& solver,
		const Device device, //the device on which the candidate coordinates are valid on
		const std::string oracleType, //the type of oracle to be selected for the evaluation
		const std::string dataset, //name of the dataset on which we are evaluation/generating data on
		const std::string filename, //name of the file for which we generated a candidate
		const App& targetApp, //complete app on the specified device (with all views)
		const App& originalApp //app with coordinates on the original device
		) {

	Json::Value all_results(Json::objectValue);
	Json::Value json_devices(Json::arrayValue);
	all_results["model"] = oracleType;
	all_results["dataset"] = dataset;
	all_results["generateData"] = false; //don't need this part anymore

		Json::Value res(Json::objectValue);
		Json::Value layouts(Json::arrayValue);
		layouts.append(candidate.ToCoordinatesJSON());
		res["layouts"] = layouts;

		res["target"] = targetApp.ToCoordinatesJSON();
		res["original"] = originalApp.ToCoordinatesJSON();

		std::string stripedFilename = filename;
		eraseSubString(stripedFilename, ".txt");
		res["filename"] = stripedFilename;

		Json::Value deviceDimensions(Json::arrayValue);
		deviceDimensions.append(device.width);
		deviceDimensions.append(device.height);

		res["dimensions"] = deviceDimensions;
		json_devices.append(res);

	all_results["devices"] = json_devices;
	const Json::Value& result = solver.sendPostToOracle(all_results);

	return result["scores"][0].asDouble();
}

//watch out: since this is going to the oracle: x, y, width, height
std::string appToString(const App& app){
	std::stringstream ss;
	auto& views = app.GetViews();
	for(unsigned i = 0; i < views.size(); i++){
		auto& view = views[i];
		ss << view.xleft << ", " << view.ytop << ", " <<  view.xright - view.xleft << ", " <<  view.ybottom -  view.ytop;
		if(i != views.size()-1){
			ss << "\n";
		}
	}
	return ss.str();
}

void writeFile(std::string filename, std::string content){
	std::ofstream out(filename);
	out << content;
	out.close();
}

void writeAppData(std::string prefix, std::string name, const std::vector<std::vector<App>>& all_candidateDeviceApps, const std::vector<App>& targetApps, const App& originalApp){
	if(all_candidateDeviceApps[0].size() != targetApps.size()){
		LOG(FATAL) << "Numbers do not match" << all_candidateDeviceApps[0].size() << " " << targetApps.size();
	}
	int numViews = all_candidateDeviceApps[0][0].GetViews().size();

	for(unsigned i = 0; i < targetApps.size(); i++){
		const App& targetApp = KeepFirstNViews(targetApps[i], numViews);
		bool containsCorrectCandidate = false;
		for(unsigned j = 0; j < all_candidateDeviceApps.size(); j++){
			int label = 0;
			if(AppMatch(all_candidateDeviceApps[j][i], targetApp)) {
				 containsCorrectCandidate = true;
				 label = 1;
			}
			std::stringstream filename;
			filename << prefix << name << "-" << i << "-" << numViews << "-" << j << "_" << label << ".txt";
			writeFile(filename.str(), appToString(all_candidateDeviceApps[j][i]));
 		}
		if(!containsCorrectCandidate){
			std::stringstream filename;
			filename << prefix << name << "-" << i << "-" << numViews << "-" << all_candidateDeviceApps.size() << "_1.txt";
			writeFile(filename.str(), appToString(targetApp));
		}
	}
	std::stringstream filename;
	filename << prefix << name << "-" << numViews << "-original.txt";
	writeFile(filename.str(), appToString(KeepFirstNViews(originalApp, numViews)));
}
