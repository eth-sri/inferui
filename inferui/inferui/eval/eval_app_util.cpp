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

#include "eval_app_util.h"
#include <gflags/gflags.h>

DEFINE_bool(user_corrects, false, "Include if user should correct the mistakes. Counts how many corrections would be needed");
DEFINE_string(experiment_type, "", "Select which experiment type you want to execute: iterativ or enumeration");
DEFINE_bool(correct_cand_exp, false, "To evaluate how often the correct candidate would have been there if all the candidate were generated in one step.");
DEFINE_bool(generate_data, false, "Mode for data generation.");


std::string ModeTypeStr(const Mode& mode) {
  switch (mode) {
    case Mode::baseline: return "baseline";
    case Mode::baselineFallback: return "baselineFallback";
    case Mode::multiAppsVerification: return "multiAppsVerification";
    case Mode::oracleMLP: return "oracleMLP";
    case Mode::oracleCnnImageOnly: return "oracleCnnImageOnly";
    case Mode::oracleCnnMLP: return "oracleCnnMLP";
    case Mode::ensembleRnnCnnAbs: return "ensembleRnnCnnAbs";
    case Mode::ensembleRnnCnnBoth: return "ensembleRnnCnnBoth";
    case Mode::doubleRNN: return "doubleRNN";

    default:
      LOG(FATAL) << "Unknown Mode";
  }
  LOG(FATAL) << "Unknown Mode";
}


std::string ModeTypeOracleStr(const Mode& mode) {
  switch (mode) {
    case Mode::oracleMLP: return "MLP";
    case Mode::oracleCnnImageOnly: return "SimpleCnn1"; //image only
    case Mode::oracleCnnMLP: return "SimpleCnn2"; //both
    case Mode::ensembleRnnCnnAbs: return "EnsembleRnnCnnAbs";
    case Mode::ensembleRnnCnnBoth: return "ensembleRnnCnnBoth";
    case Mode::doubleRNN: return "doubleRNN";
    default:
      LOG(FATAL) << "Mode is not an oracle mode";
  }
  LOG(FATAL) << "Mode is not an oracle mode";
}

bool CheckViewName(const Json::Value& rview, const View& sview) {
  return rview["id"] == StringPrintf("@+id/view%d", sview.id);
}

bool CheckPosition(int expected, int actual, int root) {
  if (actual < root) {
    return std::abs(expected - actual) <= 1;
  }
  return expected == actual;
}

bool CheckViewLocations(const Json::Value& rview, const View& sview, const View& root) {
  return CheckPosition(rview["location"][0].asInt(), sview.xleft, root.xleft) &&
         CheckPosition(rview["location"][1].asInt(), sview.ytop, root.ytop) &&
         rview["location"][2] == (sview.xright - sview.xleft) &&
         rview["location"][3] == (sview.ybottom - sview.ytop);
}

void AnalyseAppMatchLayouts(const App& ref_app, const App& syn_app){
	if (ref_app.GetViews().size() != syn_app.GetViews().size()){
     	 LOG(INFO) << "AnalyseAppMatch: Different view sizes";
		 return;
	}
	for (size_t i = 1; i < ref_app.GetViews().size(); i++) {
	    const View& ref_view = ref_app.GetViews()[i];
	    const View& syn_view = syn_app.GetViews()[i];
	    if(!(ref_view.attributes.at(Orientation::HORIZONTAL) == syn_view.attributes.at(Orientation::HORIZONTAL))){
	    	LOG(INFO) << "Different attribute_h: " << ref_view.id  << " (" << ref_view.pos << " )";
	    	LOG(INFO) << ref_view.attributes.at(Orientation::HORIZONTAL);
	    	LOG(INFO) << syn_view.attributes.at(Orientation::HORIZONTAL);

	    }
	    if(!(ref_view.attributes.at(Orientation::VERTICAL) == syn_view.attributes.at(Orientation::VERTICAL))){
	    	LOG(INFO) << "Different attribute_v: " << ref_view.id  << " (" << ref_view.pos << " )";
	    	LOG(INFO) << ref_view.attributes.at(Orientation::VERTICAL);
	    	LOG(INFO) << syn_view.attributes.at(Orientation::VERTICAL);
	    }
	  }
}


void AnalyseAppMatch(const App& ref_app, const App& syn_app) {
  if (ref_app.GetViews().size() != syn_app.GetViews().size()){
	  LOG(INFO) << "AnalyseAppMatch: Different view sizes";
	  return;
  }
  for (size_t i = 0; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if (ref_view.xleft != syn_view.xleft ||
        ref_view.xright != syn_view.xright ||
        ref_view.ytop != syn_view.ytop ||
        ref_view.ybottom != syn_view.ybottom) {
    	LOG(INFO) << "AnalyseAppMatch: Different view sizes for id: " << i;
    	LOG(INFO) << ref_view.xleft << " " << ref_view.xright << " " << ref_view.ytop << " " << ref_view.ybottom;
    	LOG(INFO) << syn_view.xleft << " " << syn_view.xright << " " << syn_view.ytop << " " << syn_view.ybottom;

    }
  }
}

//returns total number of views and how many matched
std::pair<int,int> ViewMatch(const App& ref_app, const App& syn_app) {
  if (ref_app.GetViews().size() != syn_app.GetViews().size()){
	  //maybe only report the difference...
	  return std::make_pair(0, std::max(ref_app.GetViews().size(), syn_app.GetViews().size()));
  }
  int differences = 0;
  //don't check the view 0 -> device size
  for (size_t i = 1; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if (ref_view.xleft != syn_view.xleft ||
        ref_view.xright != syn_view.xright ||
        ref_view.ytop != syn_view.ytop ||
        ref_view.ybottom != syn_view.ybottom) {
    	differences = differences + 1;
    }
  }
  //do not count view 0 as equal view ---> -1
  return std::make_pair(ref_app.GetViews().size() - 1 - differences, ref_app.GetViews().size()-1);
}

bool SingleViewMatchApprox(const View& ref_view, const View& syn_view){
	return !(std::abs(ref_view.xleft - syn_view.xleft) > 2 ||
	        std::abs(ref_view.xright - syn_view.xright) > 2 ||
	        std::abs(ref_view.ytop - syn_view.ytop) > 2 ||
	        std::abs(ref_view.ybottom - syn_view.ybottom) > 2);
}

bool SingleViewMatch(const View& ref_view, const View& syn_view){
	return ref_view.xleft == syn_view.xleft &&
			ref_view.xright == syn_view.xright &&
			ref_view.ytop == syn_view.ytop &&
			ref_view.ybottom == syn_view.ybottom;
}



//returns total number of views and how many matched
std::pair<int,int> ViewMatchApprox(const App& ref_app, const App& syn_app) {
  if (ref_app.GetViews().size() != syn_app.GetViews().size()){
	  //maybe only report the difference...
	  return std::make_pair(0, std::max(ref_app.GetViews().size(), syn_app.GetViews().size()));
  }
  int differences = 0;
  for (size_t i = 1; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if (!SingleViewMatchApprox(ref_view, syn_view))  {
    	differences = differences + 1;
    }
  }
  //do not count view 0 as equal view ---> -1
  return std::make_pair(ref_app.GetViews().size() - 1 - differences, ref_app.GetViews().size()-1);
}

std::pair<int,int> ViewMatchApproxForOrientation(const App& ref_app, const App& syn_app) {
  int horizontalMatch = 0;
  int verticalMatch = 0;
  if (ref_app.GetViews().size() != syn_app.GetViews().size()){
	  return std::make_pair(0, 0);
  }
  for (size_t i = 1; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if(std::abs(ref_view.xleft - syn_view.xleft) <= 2 && std::abs(ref_view.xright - syn_view.xright) <= 2){
    	horizontalMatch++;
    }
    if(std::abs(ref_view.ytop - syn_view.ytop) <= 2 && std::abs(ref_view.ybottom - syn_view.ybottom) <= 2){
    	verticalMatch++;
    }
  }
  return std::make_pair(horizontalMatch, verticalMatch);
}

bool AppMatchApprox(const App& ref_app, const App& syn_app) {
  if (ref_app.GetViews().size() != syn_app.GetViews().size()) return false;
  for (size_t i = 0; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];
    if (std::abs(ref_view.xleft - syn_view.xleft) > 2 ||
        std::abs(ref_view.xright - syn_view.xright) > 2 ||
        std::abs(ref_view.ytop - syn_view.ytop) > 2 ||
        std::abs(ref_view.ybottom - syn_view.ybottom) > 2)  {
      return false;
    }
  }
  return true;
}

std::pair<double, int> IntersectionOfUnion(const App& ref_app, const App& syn_app) {
  if (ref_app.GetViews().size() != syn_app.GetViews().size()) return std::make_pair(0.0,0);

  double total = 0;
  for (size_t i = 1; i < ref_app.GetViews().size(); i++) {
    const View& ref_view = ref_app.GetViews()[i];
    const View& syn_view = syn_app.GetViews()[i];

    int x_overlap = std::max(0,(std::min(ref_view.xright, syn_view.xright) - std::max(ref_view.xleft, syn_view.xleft)));
    int y_overlap = std::max(0,(std::min(ref_view.ybottom, syn_view.ybottom) - std::max(ref_view.ytop, syn_view.ytop)));

    int overlap =  x_overlap * y_overlap;
    int _union = ref_view.width() * ref_view.height() + syn_view.width() * syn_view.height() - overlap;
    total += ((double)overlap) / ((double) _union);
  }
  return std::make_pair(total,ref_app.GetViews().size()-1);
}


bool AppMatch(const App& app, const Json::Value& layout) {
  bool match = true;
  int i = 1;
  for (auto component : layout["components"]) {
    match &= CheckViewName(component, app.GetViews()[i]);
    match &= CheckViewLocations(component, app.GetViews()[i], app.GetViews()[0]);
    i++;
  }

//  match &= CheckViewLocations(layout["content_frame"], app.views[0], app.views[0]);
  return match;
}

bool CanResizeView(const App& ref_app) {
  return ref_app.IsResizable(Orientation::HORIZONTAL) || ref_app.IsResizable(Orientation::VERTICAL);
}

void TryResizeView(const App& ref_app, View& new_root, const Device& ref_device, const Device& device) {
  if (ref_app.IsResizable(Orientation::HORIZONTAL)) {
    new_root.xright += (device.width - ref_device.width);
  }
  if (ref_app.IsResizable(Orientation::VERTICAL)) {
    new_root.ybottom += (device.height - ref_device.height);
  }
}


App CuJsonToApp_reorder(const Json::Value& screen, std::vector<int>& swaps) {
  App app;
  std::vector<View> views;
  int i = 0;
  views.push_back(View(0, 0, screen["resolution"][0].asInt(), screen["resolution"][1].asInt(), "parent", i, "parent"));
  swaps.push_back(0);

  for (auto view : screen["views"]) {
	 if(view[0].asInt() == 0 &&  view[1].asInt() == 0 && view[2].asInt() == screen["resolution"][0].asInt() && view[3].asInt() == screen["resolution"][1].asInt()){
		 continue;
	 }
    i++;
    views.push_back(View(view[0].asInt(), view[1].asInt(), view[2].asInt(), view[3].asInt(), "frog", i, "frog"));
    swaps.push_back(0);
  }

  std::sort(views.begin(), views.end(), [] (const View& view1, const View& view2){
  		if(view1.is_content_frame()){
  			return true;
  		}else if(view2.is_content_frame()){
  			return false;
  		}
  		return (view1.width() * view1.height()) > (view2.width() * view2.height());
  });

  for(int i = 0; i < (int) views.size(); i++){
	  View view = views[i];
	  swaps[view.id] = i;
	  view.id = i;
	  app.AddView(std::move(view));
  }


  //just assume that views are resizable, if not layout should be generated as well
  std::vector<bool> resizable(2, true);
  app.setResizable(resizable);
  PrintApp(app, false);
  return app;
}

App CuJsonToApp_reordered(const Json::Value& screen, std::vector<int>& swapped) {
  App app;
  app.AddView(View(0, 0, screen["resolution"][0].asInt(), screen["resolution"][1].asInt(), "parent", 0, "parent"));
  int i = 0;
  std::vector<View> views;
  for (auto view : screen["views"]) {
	 if(view[0].asInt() == 0 &&  view[1].asInt() == 0 && view[2].asInt() == screen["resolution"][0].asInt() && view[3].asInt() == screen["resolution"][1].asInt()){
		 continue;
	 }
    i++;
    views.push_back(View(view[0].asInt(), view[1].asInt(), view[2].asInt(), view[3].asInt(), "frog", swapped[i], "frog"));
  }
  std::sort(views.begin(), views.end(), [] (const View& view1, const View& view2){
  		return view1.id < view2.id;
  });

  for(int i = 0; i < (int) views.size(); i++){
	  View view = views[i];
	  app.AddView(std::move(view));
  }

  //just assume that views are resizable, if not layout should be generated as well
  std::vector<bool> resizable(2, true);
  app.setResizable(resizable);
  PrintApp(app, false);
  return app;
}

App CuJsonToApp(const Json::Value& screen) {
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
  //just assume that views are resizable, if not layout should be generated as well
  std::vector<bool> resizable(2, true);
  app.setResizable(resizable);
  return app;
}

int getDeviceWidth(const Json::Value& request){
	return request["resolution"][0].asInt();
}

void JsonToApps(const Json::Value& request, App& app, std::vector<App>& apps, Device& ref_device, std::vector<Device>& devices){
	  int i = 0;
	  std::vector<int> swaps;
	  //if there are three -> reorder that it take the middle one as ref device
	  if(request["screens"].size() == 3){
		  auto layoutSmall = request["screens"][0];
		  auto layoutMiddle = request["screens"][1];
		  auto layoutBig = request["screens"][2];

		  bool firstOneSmaller = getDeviceWidth(layoutSmall) < getDeviceWidth(layoutMiddle);
		  bool secondOneSmaller = getDeviceWidth(layoutMiddle) < getDeviceWidth(layoutBig);

		  //make sure that they are ordered as expected
		  if(!(firstOneSmaller && secondOneSmaller)){
			  LOG(FATAL) << "device sizes are not ordered as expected";
		  }

		  app = CuJsonToApp_reorder(layoutMiddle, swaps);
		  ref_device = Device(layoutMiddle["resolution"][0].asInt(), layoutMiddle["resolution"][1].asInt());

		  devices.push_back(Device(layoutSmall["resolution"][0].asInt(), layoutSmall["resolution"][1].asInt()));
		  devices.push_back(Device(layoutBig["resolution"][0].asInt(), layoutBig["resolution"][1].asInt()));
		  apps.push_back(CuJsonToApp_reordered(layoutSmall, swaps));
		  apps.push_back(CuJsonToApp_reordered(layoutBig, swaps));
	  }else{
	     for (auto screen : request["screens"]) {
	       //take the first as reference app
	       if(i == 0){
	    	  //app = CuJsonToApp(screen);
	     	  app = CuJsonToApp_reorder(screen, swaps);
	     	  ref_device = Device(screen["resolution"][0].asInt(), screen["resolution"][1].asInt());
	       }else{
	    	   devices.push_back(Device(screen["resolution"][0].asInt(), screen["resolution"][1].asInt()));
	    	   //add the next one in the app vector
	    	   //const App& app = CuJsonToApp(screen);
	    	   const App& app = CuJsonToApp_reordered(screen, swaps);
	    	   apps.push_back(app);
	    	   //PrintApp(app, false);
	       }
	       i = i + 1;
	     }
	  }
}

std::pair<int, int> layoutMatch(Json::Value& ref_app, Json::Value& syn_app, bool vertical) {
	int typeMatch = 0;
	int constraintMatch = 0;
	std::string key = "horizontal";
	if(vertical){
		key = "vertical";
	}
	Json::Value ref_constraints = ref_app[key];
	Json::Value syn_constraints = syn_app[key];

	for(int i = 0; i < ref_constraints.size(); i++){
		if(ref_constraints[i]["type"] == syn_constraints[i]["type"]
		   && ref_constraints[i]["size"] == syn_constraints[i]["size"]
		   && ref_constraints[i]["val_primary"] == syn_constraints[i]["val_primary"]
		   && ref_constraints[i]["val_secondary"] == syn_constraints[i]["val_secondary"]
		   && ref_constraints[i]["bias"] == syn_constraints[i]["bias"]
		   && ref_constraints[i]["tgt_scnd"] == syn_constraints[i]["tgt_scnd"]
		   && ref_constraints[i]["tgt_prim"] == syn_constraints[i]["tgt_prim"]){
			typeMatch ++;
			constraintMatch++;
			}else if(ref_constraints[i]["type"] == syn_constraints[i]["type"]){
				typeMatch ++;
			}
	}

  return std::make_pair(typeMatch, constraintMatch);
}

Json::Value AppConstraintsToJSON(const App& app, const std::vector<int> swaps) {
	  	  	Json::Value layoutInfo(Json::objectValue);
		    Json::Value verticalConstraints(Json::arrayValue);
		    Json::Value horizontalConstraints(Json::arrayValue);

		    //necessary that the entries are in the right order (src->id = position in the json)
		    std::vector<Json::Value> h_constraints = std::vector<Json::Value>(app.GetViews().size()-1);
		    std::vector<Json::Value> v_constraints = std::vector<Json::Value>(app.GetViews().size()-1);

		    /*
		    for(int i = 0; i < app.GetViews().size(); i++){
		    	LOG(INFO)<< "here" << app.GetViews()[i].id << " ," <<  app.GetViews()[i].pos << ", " << app.seq_id_to_pos[app.GetViews()[i].id];
		    }
		    */


		    for (const View& view : app.GetViews()) {
		      if (view.is_content_frame()) continue;
		      //swaps: contains old_id -> new id
		      auto horizontalJson = view.attributes.at(Orientation::HORIZONTAL).ToJSON(app.seq_id_to_pos, swaps);
		      //LOG(INFO) << view.id << " " <<  view.pos << " " <<  app.seq_id_to_pos[view.id] << " " << swaps[app.seq_id_to_pos[view.id]];
			  //-1 since view 0 does not have a constraint
		      h_constraints[horizontalJson["srcid"].asInt()-1] = horizontalJson;
		      auto verticalJson = view.attributes.at(Orientation::VERTICAL).ToJSON(app.seq_id_to_pos, swaps);
		      v_constraints[verticalJson["srcid"].asInt()-1] = verticalJson;
		    }
		    for(auto h: h_constraints){
		    	horizontalConstraints.append(h);
		    }
		    for(auto v: v_constraints){
		    	verticalConstraints.append(v);
		    }
		    layoutInfo["horizontal"] = horizontalConstraints;
		    layoutInfo["vertical"] = verticalConstraints;

      return layoutInfo;
}




