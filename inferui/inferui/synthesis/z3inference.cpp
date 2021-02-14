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

#include "z3inference.h"
#include "inferui/layout_solver/solver.h"
#include "inferui/eval/eval_app_util.h"
#include "base/range.h"
#include <cstdlib>
#include <ctime>
#include <set>
#include <atomic>


const std::regex ConstraintData::constraint_name_regex = std::regex("([^_]*)_(\\d+)_(\\d+)_(\\d+)_(\\d+)_?(\\d+)?");
DEFINE_uint64(cand_num, 4, "Square root of the number of candidates to be generated");

expr round_real2int(const expr &x) {
  Z3_ast r = Z3_mk_real2int(x.ctx(), x + x.ctx().real_val(1,2));
  return expr(x.ctx(), r);
}

const std::vector<ConstraintRelationalFixedSize> ConstraintRelationalFixedSize::CONSTRAINTS = {
    ConstraintRelationalFixedSize(std::make_pair(ConstraintType::L2L, ConstraintType::T2T), [](const Z3View* src, const Z3View* tgt, const Z3View* dummy){
      return src->position_start_v == tgt->position_start_v + src->margin_start_v;
    }),
    ConstraintRelationalFixedSize(std::make_pair(ConstraintType::L2R, ConstraintType::T2B), [](const Z3View* src, const Z3View* tgt, const Z3View* dummy){
      return src->position_start_v == tgt->position_end_v + src->margin_start_v;
    }),
    ConstraintRelationalFixedSize(std::make_pair(ConstraintType::R2L, ConstraintType::B2T), [](const Z3View* src, const Z3View* tgt, const Z3View* dummy){
      return src->position_end_v == tgt->position_start_v - src->margin_end_v;
    }),
    ConstraintRelationalFixedSize(std::make_pair(ConstraintType::R2R, ConstraintType::B2B), [](const Z3View* src, const Z3View* tgt, const Z3View* dummy){
      return src->position_end_v == tgt->position_end_v - src->margin_end_v;
    })
};

const std::vector<ConstraintCenteringFixedSize> ConstraintCenteringFixedSize::CONSTRAINTS = {
    ConstraintCenteringFixedSize(std::make_pair(ConstraintType::L2LxR2R, ConstraintType::T2TxB2B), [](const Z3View* src, const Z3View* start, const Z3View* end){
      expr bias = src->GetBiasExpr();
      expr res = round_real2int(
          ((1 - bias) * (start->position_start_v + src->margin_start_v) + bias * (end->position_end_v - src->margin_end_v))
          - ((1 - bias) * src->position_start_v + bias * src->position_end_v)) == 0;
      if (start->pos == 0 && end->pos == 0) {
        // bug in Android
        res = res && src->margin_start_v <= src->position_start_v - start->position_start_v &&
            src->margin_end_v <= end->position_end_v - src->position_end_v;
      }
      return res;

    }),
    ConstraintCenteringFixedSize(std::make_pair(ConstraintType::L2LxR2L, ConstraintType::T2TxB2T), [](const Z3View* src, const Z3View* start, const Z3View* end){
      expr bias = src->GetBiasExpr();
      if (*start == *end) {
        // When both anchors are the same the margin is ignored by the layout solver
        return src->margin_start_v == 0 && src->margin_end_v == 0 &&
            round_real2int(
                ((1 - bias) * start->position_start_v + bias * end->position_start_v)
                - ((1 - bias) * src->position_start_v + bias * src->position_end_v)) == 0;
      } else {
        return round_real2int(
            ((1 - bias) * (start->position_start_v + src->margin_start_v) + bias * (end->position_start_v - src->margin_end_v))
            - ((1 - bias) * src->position_start_v + bias * src->position_end_v)) == 0;
      }
    }),
    ConstraintCenteringFixedSize(std::make_pair(ConstraintType::L2RxR2R, ConstraintType::T2BxB2B), [](const Z3View* src, const Z3View* start, const Z3View* end){
      expr bias = src->GetBiasExpr();
      if (*start == *end) {
        // When both anchors are the same the margin is ignored by the layout solver
        return src->margin_start_v == 0 && src->margin_end_v == 0 &&
            round_real2int(
                ((1 - bias) * start->position_end_v + bias * end->position_end_v)
                - ((1 - bias) * src->position_start_v + bias * src->position_end_v)) == 0;
      } else {
        return round_real2int(
            ((1 - bias) * (start->position_end_v + src->margin_start_v) + bias * (end->position_end_v - src->margin_end_v))
            - ((1 - bias) * src->position_start_v + bias * src->position_end_v)) == 0;
      }
    }),
    ConstraintCenteringFixedSize(std::make_pair(ConstraintType::L2RxR2L, ConstraintType::T2BxB2T), [](const Z3View* src, const Z3View* start, const Z3View* end){
      expr bias = src->GetBiasExpr();
      return round_real2int(
          ((1 - bias) * (start->position_end_v + src->margin_start_v) + bias * (end->position_start_v - src->margin_end_v))
          - ((1 - bias) * src->position_start_v + bias * src->position_end_v)) == 0;
    })
};


const std::vector<ConstraintCenteringMatchConstraint> ConstraintCenteringMatchConstraint::CONSTRAINTS = {
    ConstraintCenteringMatchConstraint(std::make_pair(ConstraintType::L2LxR2R, ConstraintType ::T2TxB2B), [](const Z3View* src, const Z3View* start, const Z3View* end){
      return src->position_start_v == start->position_start_v + src->margin_start_v &&
             src->position_end_v == end->position_end_v - src->margin_end_v;
    }),
    ConstraintCenteringMatchConstraint(std::make_pair(ConstraintType::L2LxR2L, ConstraintType ::T2TxB2T), [](const Z3View* src, const Z3View* start, const Z3View* end){
      return src->position_start_v == start->position_start_v + src->margin_start_v &&
             src->position_end_v == end->position_start_v - src->margin_end_v;
    }),
    ConstraintCenteringMatchConstraint(std::make_pair(ConstraintType::L2RxR2R, ConstraintType ::T2BxB2B), [](const Z3View* src, const Z3View* start, const Z3View* end){
      return src->position_start_v == start->position_end_v + src->margin_start_v &&
             src->position_end_v == end->position_end_v - src->margin_end_v;
    }),
    ConstraintCenteringMatchConstraint(std::make_pair(ConstraintType::L2RxR2L, ConstraintType ::T2BxB2T), [](const Z3View* src, const Z3View* start, const Z3View* end){
      return src->position_start_v == start->position_end_v + src->margin_start_v &&
             src->position_end_v == end->position_start_v - src->margin_end_v;
    }),
};

std::string StatusStr(const Status& status) {
  switch (status) {
    case Status::SUCCESS:
      return "SUCCESS";
    case Status::UNSAT:
      return "UNSAT";
    case Status::INVALID:
      return "INVALID";
    case Status::TIMEOUT:
      return "TIMEOUT";
    case Status::UNKNOWN:
      return "UNKNOWN";

  }
  LOG(FATAL) << "Unknown status: " << static_cast<int>(status);
}


bool TryResizeView(const App& ref_app, View& new_root, const Device& ref_device, const Device& device, const Orientation& orientation) {
  bool resized = false;
  if (orientation == Orientation::HORIZONTAL && (ref_app.IsResizable(Orientation::HORIZONTAL) || device.width < ref_app.GetViews()[0].width())) {
    new_root.xright += (device.width - ref_device.width);
    resized = true;
  }
  if (orientation == Orientation::VERTICAL && (ref_app.IsResizable(Orientation::VERTICAL) || device.height < ref_app.GetViews()[0].height())) {
    new_root.ybottom += (device.height - ref_device.height);
    resized = true;
  }
  return resized;
}

std::pair<int, int> FullSynthesis::AlignmentPoints(int xleft, int xright, int yleft, int yright) {
  if (xright <= yleft) {
// y completely to the right
    return std::make_pair(xright, yleft);
  } else if (yright <= xleft) {
// y completely to the left
    return std::make_pair(xleft, yright);
  } else {


    if (xleft <= yleft && yright <= xright) {
      // y contained in x
      if (std::abs(xleft - yleft) < std::abs(xright - yright)) {
        return std::make_pair(xleft, yleft);
      } else {
        return std::make_pair(xright, yright);
      }

    } else if (yleft <= xleft && xright <= yright) {
      // x contained in y
      if (std::abs(xleft - yleft) < std::abs(xright - yright)) {
        return std::make_pair(xleft, yleft);
      } else {
        return std::make_pair(xright, yright);
      }
    } else {
      // otherwise they intersect
      return std::make_pair(-1, -1);
    }
  }
}


App ResizeApp(
    const App& ref,
    const Device& ref_device,
    const Device& target_device) {

  App app;
  for (const View& view : ref.GetViews()) {
    if (view.is_content_frame()) {
      app.AddView(View(view.xleft, view.ytop, view.xright, view.ybottom, view.name, view.id));
    } else {
      app.AddView(View(-1, -1, -1, -1, view.name, view.id));
    }
  }

  TryResizeView(ref, app.GetViews()[0], ref_device, target_device, Orientation::HORIZONTAL);
  TryResizeView(ref, app.GetViews()[0], ref_device, target_device, Orientation::VERTICAL);
  return app;
}

App ResizeApp(const App& ref, float ratio) {
  App app;

  const View& content_frame = ref.GetViews()[0];
  int xoffset = content_frame.width() * ratio;
  xoffset -= xoffset % 2;
  int yoffset = content_frame.height() * ratio;
  yoffset -= yoffset % 2;

  for (const View& view : ref.GetViews()) {
    if (view.is_content_frame()) {
      app.AddView(View(view.xleft, view.ytop, view.xright + xoffset, view.ybottom + yoffset, view.name, view.id));
    } else {
      app.AddView(View(-1, -1, -1, -1, view.name, view.id));
    }
  }
  return app;
}

Status FullSynthesis::SynthesizeLayoutMultiDevice(App& app,
                                   const Device& ref_device,
                                   const std::vector<Device>& devices) const {
  std::vector<App> device_apps;
  for (size_t device_id = 0; device_id < devices.size(); device_id++) {
    const Device &target_device = devices[device_id];
    device_apps.emplace_back(ResizeApp(app, ref_device, target_device));
  }
  Timer timer;
  Status status = SynthesizeMultiDevice(app, Orientation::VERTICAL, nullptr, ref_device, device_apps, timer);
  if (status == Status::SUCCESS) {
    status = SynthesizeMultiDevice(app, Orientation::HORIZONTAL, nullptr, ref_device, device_apps, timer);
  }
  return status;
}

Status FullSynthesis::SynthesizeLayoutMultiDevice(
    App& app,
    const ProbModel* model,
    const Device& ref_device,
    const std::vector<Device>& devices) const {

  std::vector<App> device_apps;
  for (size_t device_id = 0; device_id < devices.size(); device_id++) {
    const Device &target_device = devices[device_id];
    device_apps.emplace_back(ResizeApp(app, ref_device, target_device));
  }

  Timer timer;
  timer.StartScope("prob_model");
//  LOG(INFO) << "Initialize Prob Model...";
  AttrScorer scorer_vertical(model, app, Orientation::VERTICAL);
  AttrScorer scorer_horizontal(model, app, Orientation::HORIZONTAL);
//  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
  timer.EndScope();

  Status status = SynthesizeMultiDevice(app, Orientation::VERTICAL, &scorer_vertical, ref_device, device_apps, timer);
  if (status == Status::SUCCESS) {
    status = SynthesizeMultiDevice(app, Orientation::HORIZONTAL, &scorer_horizontal, ref_device, device_apps, timer);
  }

  timer.Dump();
  return status;
}

Status FullSynthesis::SynthesizeLayoutMultiDeviceProb(
    App& app,
    const ProbModel* model,
    const Device& ref_device,
    std::vector<App>& device_apps,
    bool opt) const {
  Timer timer;
  timer.StartScope("prob_model");
  AttrScorer scorer_vertical(model, app, Orientation::VERTICAL);
  AttrScorer scorer_horizontal(model, app, Orientation::HORIZONTAL);
  timer.EndScope();

  Status status = SynthesizeMultiDeviceProb(app, Orientation::VERTICAL, &scorer_vertical, ref_device, device_apps, timer, true, !device_apps.empty(), opt);
  if (status == Status::SUCCESS) {
    status = SynthesizeMultiDeviceProb(app, Orientation::HORIZONTAL, &scorer_horizontal, ref_device, device_apps, timer, true, !device_apps.empty(), opt);
  }

  timer.Dump();
  return status;
}

Status FullSynthesis::SynthesizeLayoutMultiDeviceProb(
    App& app,
    const ProbModel* model,
    const Device& ref_device,
    const std::vector<Device>& devices,
    bool opt) const {

  PrintApp(app, false);
  std::vector<App> device_apps;
  for (size_t device_id = 0; device_id < devices.size(); device_id++) {
    const Device &target_device = devices[device_id];
    device_apps.emplace_back(ResizeApp(app, ref_device, target_device));
  }

  Timer timer;
  timer.StartScope("prob_model");
//  LOG(INFO) << "Initialize Prob Model...";
  AttrScorer scorer_vertical(model, app, Orientation::VERTICAL);
  AttrScorer scorer_horizontal(model, app, Orientation::HORIZONTAL);
//  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
  timer.EndScope();
  Status status;

   status = SynthesizeMultiDeviceProb(app, Orientation::VERTICAL, &scorer_vertical, ref_device, device_apps, timer, false, !devices.empty(), opt);
   if (status == Status::SUCCESS) {
	   status = SynthesizeMultiDeviceProb(app, Orientation::HORIZONTAL, &scorer_horizontal, ref_device, device_apps, timer, false, !devices.empty(), opt);
   }

  timer.Dump();
//  if (status == Status::SUCCESS) {
//    PrintApp(app);
//  }
  return status;
}


std::pair<double, double> getProb(const App& app){
	std::vector<double> layoutScores;
	double score_v = 0.0;
	double score_h = 0.0;
	for (const View& view : app.GetViews()) {
		if(view.id == 0){
			continue;
		}
		score_h += view.attributes.at(Orientation::HORIZONTAL).prob;
		score_v += view.attributes.at(Orientation::VERTICAL).prob;
	}
	return std::make_pair(score_h, score_v);
}

std::vector<std::pair<double, double>> getProbIndividual(const App& app){
	std::vector<std::pair<double, double>> layoutScores;
	for (const View& view : app.GetViews()) {
		if(view.id == 0){
			layoutScores.push_back(std::make_pair(0,0));
			continue;
		}
		layoutScores.push_back(std::make_pair(view.attributes.at(Orientation::HORIZONTAL).prob, view.attributes.at(Orientation::VERTICAL).prob));
	}
	return layoutScores;
}

//with prob from prob. model as tiebreaker, normally the prob model is already at the beginning
int selectCandidate(int selectedCandidate, const std::vector<double>& scores, const std::vector<App>& candidates){

	if(scores.size() <= 1){
		LOG(FATAL) << "Error. Scores should be bigger than 0";
	}

	double min = 2.0;
	double max = -1.0;
	for(unsigned i = 0; i < scores.size(); i++){
		if(scores[i] > 1 || scores[i] < 0){
			LOG(FATAL) << "Invalid input.";
		}
		min = std::min(min, scores[i]);
		max = std::max(max, scores[i]);
	}
	//maxbe come up with better heuristic here
	if(max - min > 0.01){
		return selectedCandidate;
	}

	std::vector<double> layoutScores;
	double minLayoutScore = 10; //assuming prob is always negative
	int minIndex = 0;

	for(unsigned i = 0; i < candidates.size(); i++){
		const App& app = candidates[i];
		auto res = getProb(app);
		double score = res.first + res.second;

		if(score > minLayoutScore){
			minLayoutScore = score;
			minIndex = i;
		}
	}

	if(minIndex != selectedCandidate){
		LOG(INFO) << "Decided differently" << scores[minIndex] << " " <<  scores[selectedCandidate];
	}
	return minIndex;
}


void eraseSubStr(std::string &orginal, const std::string& toErase)
{
	// Search for the substring in string
	size_t pos = orginal.find(toErase);

	if (pos != std::string::npos)
	{
		// If found then erase it from string
		orginal.erase(pos, toErase.length());
	}
}

//checks which possible app the oracle prefers
//returns the id of the selected app
//candidates contain layout information
std::pair<int, std::vector<int>> askOracle(
		const std::vector<std::vector<App>>& candidates_resized,
		const std::vector<App>& candidates, Solver& _solver,
		const std::vector<Device>& devices, const std::string oracleType, const std::string dataset,
		const std::string filename, std::vector<App> debugApps,
		const App& originalApp,
		const Json::Value targetXML) {

	Json::Value all_results(Json::objectValue);
	Json::Value json_devices(Json::arrayValue);
	all_results["model"] = oracleType;
	all_results["dataset"] = dataset;
	all_results["targetXML"] = targetXML;
	all_results["generateData"] = FLAGS_generate_data;

	for (unsigned j = 0; j < devices.size(); j++) {
		Json::Value res(Json::objectValue);
		Json::Value layouts(Json::arrayValue);
		for (unsigned i = 0; i < candidates_resized.size(); i++) {
			auto prob_layout = getProb(candidates[i]);
			//TODO (larissa), this is dangerous, in case reordering takes place -> layout info might not be correct
			auto prob_layout_individual = getProbIndividual(candidates[i]);
			layouts.append(
					candidates_resized[i][j].ToCoordinatesJSON(i, candidates[i], AppConstraintsToJSON(candidates[i]),
							prob_layout, prob_layout_individual));

		}
		res["layouts"] = layouts;

		res["target"] = debugApps[j].ToCoordinatesJSON();
		res["original"] = originalApp.ToCoordinatesJSON();

		std::string stripedFilename = filename;
		eraseSubStr(stripedFilename, ".txt");
		res["filename"] = stripedFilename;

		Json::Value deviceDimensions(Json::arrayValue);
		deviceDimensions.append(devices[j].width);
		deviceDimensions.append(devices[j].height);

		res["dimensions"] = deviceDimensions;
		json_devices.append(res);
	}

	all_results["devices"] = json_devices;
	const Json::Value& result = _solver.sendPostToOracle(all_results);

	std::vector<double> results;
	for (auto score : result["scores"]) {
		results.push_back(score.asDouble());
	}
	std::vector<int> maxIndexes;
	for (auto score : result["results"]) {
		maxIndexes.push_back(score.asInt());
	}

	return std::make_pair(result["result"].asInt(), maxIndexes);
}


Status FullSynthesis::computeCandidates(
		int numberOfCandidates,
		std::vector<App>& candidates,
		std::vector<std::vector<App>>& candidates_resized,
		const App& app,
		const std::vector<App>& device_apps,
		const Device& ref_device,
		const std::vector<Device>& devices,
		const AttrScorer* scorer_vertical,
		const AttrScorer* scorer_horizontal,
		Timer& timer, bool opt) const{

	std::vector<App> verticalCandidates;
	std::vector<std::vector<App>> verticalCandidates_resized;
	std::vector<App> verticalblockedApps;

	std::vector<App> horizontalCandidates;
	std::vector<std::vector<App>> horizontalCandidates_resized;
	std::vector<App> horizontalblockedApps;

	int distinguishingDevice = 0;
	//PrintApp(app, false);
	//PrintApp(device_apps[0], false);

	for (int i = 0; i < numberOfCandidates; i++) {
		std::vector<App> target_apps;
		for (size_t device_id = 0; device_id < devices.size(); device_id++) {
			const Device& target_device = devices[device_id];
			LOG(INFO) << "Adding target device" << target_device.width << " " << target_device.height;
			target_apps.emplace_back(ResizeApp(app, ref_device, target_device));
		}
		App candidate = app;

		std::pair<Status, ConstraintMap> res_v = SynthesizeDeviceProbOracle(
				candidate, Orientation::VERTICAL, scorer_vertical, ref_device,
				device_apps, target_apps, timer, opt, verticalblockedApps);

		if (res_v.first != Status::SUCCESS) {
			if (i == 0) { //break the CEGIS loop if we can not even generate one example which satisfies the view constraints
				LOG(INFO) << "No vertical constraint";
				return res_v.first;
			} else {
				break;
			}
		}

		verticalCandidates.push_back(candidate);
		verticalblockedApps.push_back(target_apps[distinguishingDevice]);
		verticalCandidates_resized.push_back(target_apps);
	}

	for (int i = 0; i < numberOfCandidates; i++) {
		std::vector<App> target_apps;
		for (size_t device_id = 0; device_id < devices.size(); device_id++) {
			const Device& target_device = devices[device_id];
			LOG(INFO) << "Adding target device" << target_device.width << " " << target_device.height;
			target_apps.emplace_back(ResizeApp(app, ref_device, target_device));
		}
		App candidate = app;

		std::pair<Status, ConstraintMap> res_h = SynthesizeDeviceProbOracle(
				candidate, Orientation::HORIZONTAL, scorer_horizontal,
				ref_device, device_apps, target_apps, timer, opt, horizontalblockedApps);

		if (res_h.first != Status::SUCCESS) {
			if (i == 0) { //break the CEGIS loop if we can not even generate one example which satisfies the view constraints
				LOG(INFO) << "No horizontal constraint";
				return res_h.first;
			} else {
				break;
			}
		}
		horizontalCandidates.push_back(candidate);
		horizontalblockedApps.push_back(target_apps[distinguishingDevice]);
		horizontalCandidates_resized.push_back(target_apps);
	}

	for(int i = 0; i < (int)verticalCandidates.size(); i++){
		for(int j = 0; j < (int)horizontalCandidates.size(); j++){
			candidates.push_back(App(verticalCandidates[i], horizontalCandidates[j]));
			std::vector<App> merged;
			for(int k = 0; k < (int)verticalCandidates_resized[i].size(); k++){
				merged.push_back(App(verticalCandidates_resized[i][k], horizontalCandidates_resized[j][k]));
			}
			candidates_resized.push_back(merged);
		}
	}

	  int different = computeMatchings(candidates, candidates_resized, distinguishingDevice);

	  //to verify if it worked properly
	  if(different != (int)candidates.size()){
		  LOG(FATAL) << "Different is not equal to the candidates size " << different << " " << candidates.size();
	  }

	return Status::SUCCESS;
}

//to verify that computed candidates are different
int FullSynthesis::computeMatchings(
		std::vector<App>& candidates,
		std::vector<std::vector<App>>& candidates_resized,
		int deviceId,
		bool checkLayouts) const {

	//computes how many are different to the first candidate
	int different = 1;

	//first candidate
	App resized_app = candidates_resized[0][deviceId];

	//the first candidate is the unconstrained one
	for (unsigned i = 1; i < candidates_resized.size(); i++) {
		App second_resized_app = candidates_resized[i][deviceId];
		//AppMatchApprox -> get maximum number from there
		bool isMatch = AppMatch(second_resized_app, resized_app);
		if (!isMatch) {
			different++;
			AnalyseAppMatch(resized_app, second_resized_app);
			if(checkLayouts){
				AnalyseAppMatchLayouts(candidates[0], candidates[i]);
			}
		}
	}
	return different;
}


Status FullSynthesis::SynthesizeLayoutMultiDeviceProbUser(
    App& app,
    const ProbModel* model,
    const Device& ref_device,
    std::vector<App>& device_apps,
    bool opt,
    bool robust,
    const std::function<bool(const App&)>& cb) const {

  LOG(INFO) << "============================================================";
  LOG(INFO) << "============================================================";
  PrintApp(app, false);

  Timer timer;
  timer.StartScope("prob_model");
//  LOG(INFO) << "Initialize Prob Model...";
  AttrScorer scorer_vertical(model, app, Orientation::VERTICAL);
  AttrScorer scorer_horizontal(model, app, Orientation::HORIZONTAL);
//  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
  timer.EndScope();
  Status status;
  do {
    status = SynthesizeMultiDeviceProb(app, Orientation::VERTICAL, &scorer_vertical, ref_device, device_apps,
                                       timer, true, robust, opt);
    if (status == Status::SUCCESS) {
      status = SynthesizeMultiDeviceProb(app, Orientation::HORIZONTAL, &scorer_horizontal, ref_device, device_apps,
                                         timer, true, robust, opt);
    }
    for (const App& device_app : device_apps) {
      PrintApp(device_app, false);
    }
  } while (status == Status::SUCCESS && cb(app));

  timer.Dump();
//  if (status == Status::SUCCESS) {
//    PrintApp(app);
//  }
  return status;
}

Status FullSynthesis::SynthesizeLayoutIterative(
    App& app,
    const ProbModel* model,
    std::vector<App>& device_apps,
    bool opt,
    int max_candidates,
    const std::function<bool(int, const App&, const std::vector<App>&)>& candidate_cb,
    const std::function<std::vector<App>(int, const App&)>& predict_cb,
    const std::function<void(int)>& iter_cb) const {

  App cur_app;
  cur_app.setResizable(app.resizable);
  std::vector<App> cur_device_apps(device_apps.size());

  for (auto view_id : util::lang::indices(app.GetViews())) {
    cur_app.GetViews().push_back(app.GetViews()[view_id]);
    for (int device_id : util::lang::indices(device_apps)) {
      cur_device_apps[device_id].GetViews().push_back(device_apps[device_id].GetViews()[view_id]);
    }
    if (view_id == 0) continue; //content view

//    LOG(INFO) << "Iter: " << cur_app.GetViews().size();
//    PrintApp(cur_device_apps[0], false);

    bool candidate_selected = false;
    int num_candidates = 0;
    Status status = SynthesizeLayoutMultiAppsProbSingleQueryCandidates(cur_app, model, cur_device_apps, opt,
        [&num_candidates, &candidate_selected, max_candidates, candidate_cb](const App& candidate_app, const std::vector<App>& candidate_device_apps) {
      num_candidates++;
      if (!candidate_cb(num_candidates, candidate_app, candidate_device_apps)) {
        candidate_selected = true;
        return false;
      }
      return num_candidates < max_candidates;
    });

    if (num_candidates == 0) {
      CHECK_NE(status, Status::SUCCESS);
      return status;
    }

    if (!candidate_selected) {
      // predict_cb can return one of the candidates passed using candidate_cb but also something else (e.g, ground-truth if available)
      cur_device_apps = predict_cb(view_id + 1, cur_app);
    }
    iter_cb(view_id + 1);
  }

  // update app with the result
  app = cur_app;
  // run the synthesis with all the inputs
  return SynthesizeLayoutMultiAppsProbSingleQuery(app, model, cur_device_apps, opt);
}

Status FullSynthesis::SynthesizeLayoutMultiAppsProbSingleQueryCandidates(
    App& app,
    const ProbModel* model,
    std::vector<App>& device_apps,
    bool opt,
    const std::function<bool(const App&, const std::vector<App>&)>& cb) const {

  Timer timer;
  timer.StartScope("prob_model");
  OrientationContainer<AttrScorer> scorers(
      AttrScorer(model, app, Orientation::HORIZONTAL),
      AttrScorer(model, app, Orientation::VERTICAL)
  );
  timer.EndScope();

  BlockingConstraintsHelper blocking_helper(device_apps);
  Status status;
  do {
    blocking_helper.ResetViews(device_apps);

//    LOG(INFO) << "====";
//    PrintApp(device_apps[0], false);

    status = SynthesizeMultiDeviceProbSingleQuery(app, scorers, device_apps, timer, opt, &blocking_helper);
    if (status != Status::SUCCESS) {
      return status;
    }
    blocking_helper.AddViews(device_apps);
  } while (cb(app, device_apps));

  timer.Dump();
  return status;
}

Status FullSynthesis::SynthesizeLayoutMultiAppsProbSingleQuery(
    App& app,
    const ProbModel* model,
    std::vector<App>& device_apps,
    bool opt) const {
  LOG(INFO) << "============================================================";
  LOG(INFO) << "============================================================";
  PrintApp(app, false);

  Timer timer;
  timer.StartScope("prob_model");
  OrientationContainer<AttrScorer> scorers(
      AttrScorer(model, app, Orientation::HORIZONTAL),
      AttrScorer(model, app, Orientation::VERTICAL)
  );
  timer.EndScope();
  Status status = SynthesizeMultiDeviceProbSingleQuery(app, scorers, device_apps, timer, opt);

  timer.Dump();
  return status;
}

Status FullSynthesis::SynthesizeLayoutMultiAppsProb(
    App& app,
    const ProbModel* model,
    const Device& ref_device,
    std::vector<App>& device_apps,
    bool opt) const {

  LOG(INFO) << "============================================================";
  LOG(INFO) << "============================================================";
  PrintApp(app, false);

  Timer timer;
  timer.StartScope("prob_model");

  OrientationContainer<AttrScorer> scorers(
      AttrScorer(model, app, Orientation::HORIZONTAL),
      AttrScorer(model, app, Orientation::VERTICAL)
  );
  timer.EndScope();
  Status status;

  status = SynthesizeMultiDeviceProb(app, Orientation::VERTICAL, &scorers[Orientation::VERTICAL], ref_device, device_apps,
                                     timer, true, false, opt);
  if (status == Status::SUCCESS) {
    status = SynthesizeMultiDeviceProb(app, Orientation::HORIZONTAL, &scorers[Orientation::HORIZONTAL], ref_device, device_apps,
                                       timer, true, false, opt);
  }

  timer.Dump();
//  if (status == Status::SUCCESS) {
//    PrintApp(app);
//  }
  return status;
}



std::vector<Z3View*> GetUnsatViews(solver& s, std::vector<Z3View>& z3_views) {
  std::vector<Z3View*> res;
  expr_vector core = s.unsat_core();
//  LOG(INFO) << core;
//  LOG(INFO) << "size: " << core.size();
  for (Z3View& view : z3_views) {
    if (view.pos == 0) continue;

    std::string name = view.GetConstraintsSatisfied().to_string();
    bool sat = false;
    for (unsigned i = 0; i < core.size(); i++) {
      if (core[i].to_string() == name) {
        sat = true;
        break;
      }
    }
    if (sat) {
      res.push_back(&view);
    }
  }

  return res;
}

expr_vector GetAssumptions(context& c, OrientationContainer<std::vector<Z3View>>& z3_views) {
  expr_vector assumptions(c);
  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    for (Z3View &view : z3_views[orientation]) {
      if (view.pos == 0) continue;
      //LOG(INFO) << "LARISSA, constraint satisfied" << view.GetConstraintsSatisfied();
      assumptions.push_back(view.GetConstraintsSatisfied());
    }
  }
  return assumptions;
}

expr_vector GetAssumptions(context& c, std::vector<Z3View>& z3_views) {
  expr_vector assumptions(c);
  for (Z3View& view : z3_views) {
    if (view.pos == 0) continue;
    //LOG(INFO) << "LARISSA, constraint satisfied" << view.GetConstraintsSatisfied();
    assumptions.push_back(view.GetConstraintsSatisfied());
  }
  return assumptions;
}



Status FullSynthesis::SynthesizeMultiDevice(
    App& app,
    const Orientation& orientation,
    const AttrScorer* scorer,
    const Device& ref_device,
    std::vector<App>& device_apps,
    Timer& timer) const {
  timer.StartScope("add_constraints");
  LOG(INFO) << "Syn: " << orientation;
  LOG(INFO) << "Initialize Constraints...";
  context c;
  solver s(c);

  CHECK(scorer == nullptr);

  std::vector<std::vector<Z3View>> z3_views_devices;
  // single device specification

  std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

  AddPositionConstraints(s, z3_views);
  AddAnchorConstraints(s, z3_views);

  ForEachNonRootView(s, orientation, z3_views, AddFixedSizeRelationalConstraint<solver>,
                     [&scorer, &s](const std::string& name, const Z3View& src) {
                       return true;
                     });

  ForEachNonRootView(s, orientation, z3_views, AddFixedSizeCenteringConstraint<solver>,
                     [&scorer, &s](const std::string &name, const Z3View &src) {
                       return true;
                     });

  ForEachNonRootView(s, orientation, z3_views, AddMatchConstraintCenteringConstraint<solver>,
                     [&scorer, &s](const std::string& name, const Z3View& src){
                       return true;
                     });



  // multi device constraints
  {
    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {

      App& device_app = device_apps[device_id];
      LOG(INFO) << "\tdevice: " << device_app.GetViews()[0];

      //TODO: Check for screen with same dimensions
//      if ((orientation == Orientation::HORIZONTAL && device_app.views[0].width() == app.views[0].width())) {
//          continue;
//      }

      z3_views_devices.emplace_back(Z3View::ConvertViews(device_app.GetViews(), orientation, c, device_id + 1));
      std::vector<Z3View>& z3_views_device = z3_views_devices[z3_views_devices.size() - 1];
      // Set fixed size of the root layout
      s.add(z3_views_device[0].position_start_v == z3_views_device[0].start);
      s.add(z3_views_device[0].position_end_v == z3_views_device[0].end);

      for (size_t i = 1; i < z3_views.size(); i++) {
        if (z3_views[i].end - z3_views[i].start > 0) {
          s.add(
              (z3_views_device[i].position_end_v - z3_views_device[i].position_start_v > (z3_views[i].end - z3_views[i].start) / 2) &&
              (z3_views_device[i].position_end_v - z3_views_device[i].position_start_v < (z3_views[i].end - z3_views[i].start) * 2)
          );
        }
      }

      AssertNotOutOfBounds(s, z3_views_device);

      AssertKeepsIntersection(s, app, z3_views, z3_views_device);
      AssertKeepsCentering(s, app, z3_views, z3_views_device);
      AssertKeepsMargins(s, app, z3_views, z3_views_device);
      if (orientation == Orientation::HORIZONTAL) {
        AssertKeepsSizeRatio(s, app, z3_views, z3_views_device, device_app);
      }

      ForEachNonRootView(s, orientation, z3_views_device, AddFixedSizeRelationalConstraint<solver>,
                         [&s, &app, &orientation](const std::string &name, const Z3View &src) {
                           expr cond = s.ctx().bool_const(name.c_str());
                           int value = (orientation == Orientation::HORIZONTAL) ? app.GetViews()[src.pos].width()
                                                                                : app.GetViews()[src.pos].height();
                           s.add(implies(cond, src.position_start_v + value == src.position_end_v));
                           return true;
                         });

      ForEachNonRootView(s, orientation, z3_views_device, AddFixedSizeCenteringConstraint<solver>,
                         [&s, &app, &orientation](const std::string &name, const Z3View &src) {
                           expr cond = s.ctx().bool_const(name.c_str());
                           int value = (orientation == Orientation::HORIZONTAL) ? app.GetViews()[src.pos].width()
                                                                                : app.GetViews()[src.pos].height();
                           s.add(implies(cond, src.position_start_v + value == src.position_end_v));
                           return true;
                         });

      ForEachNonRootView(s, orientation, z3_views_device, AddMatchConstraintCenteringConstraint<solver>,
                         [&s](const std::string &name, const Z3View &src) {
                           s.add(implies(cond, src.position_end_v - src.position_start_v >= 0));
                           return true;
                         });

    }
  }

  FinishedAddingConstraints(s, z3_views);

//  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
//  LOG(INFO) << "Solving...";
  timer.EndScope();
  timer.StartScope("solving");

//    LOG(INFO) << s;

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
  p.set(":unsat-core", true);
//  p.set("priority",c.str_symbol("pareto"));
  s.set(p);

  Timer check_timer;
  check_timer.Start();
  check_result res = s.check(GetAssumptions(c, z3_views));
//  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
//  VLOG(2) << res;
  LOG(INFO) << "check_sat: " << res;

  timer.EndScope();


  if (res != check_result::sat) {
    LOG(INFO) << res << " for:";
    for (size_t i = 0; i < app.GetViews().size(); i++) {
      LOG(INFO) << '\t' << app.GetViews()[i];
    }

    if (check_timer.GetMilliSeconds() > timeout) {
      return Status::TIMEOUT;
    }
    if (res == check_result::unsat) {

      expr_vector core = s.unsat_core();
      LOG(INFO) << core;
      LOG(INFO) << "size: " << core.size();
      for (unsigned i = 0; i < core.size(); i++) {
        LOG(INFO)  << core[i];
      }

//      LOG(INFO) << "Unsat views: " << JoinInts(GetUnsatViews(s, z3_views), ',');

      return Status::UNSAT;
    }
    CHECK(res == check_result::unknown);
    return Status::UNKNOWN;
  }

  model m = s.get_model();
//  LOG(INFO) << m << "\nDone";
  LOG(INFO) << "Generating Output...";
//  timer.Start();

  timer.StartScope("generating_output");
  for (Z3View& view : z3_views) {
    if (view.pos == 0) continue;

    view.AssignModel(m, orientation, app.GetViews());
  }
//  PrintApp(app, false);

  for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
    App& device_app = device_apps[device_id];
    std::vector<Z3View>& z3_views_device = z3_views_devices[device_id];

    CHECK_EQ(device_app.GetViews().size(), z3_views_device.size());
    for (size_t view_id = 1; view_id < z3_views_device.size(); view_id++) {

      Z3View& z3_view = z3_views_device[view_id];
      View& view = device_app.GetViews()[view_id];
      z3_view.AssignPosition(m);
      if (orientation == Orientation::HORIZONTAL) {
        view.xleft = z3_view.start;
        view.xright = z3_view.end;
      } else {
        view.ytop = z3_view.start;
        view.ybottom = z3_view.end;
      }
    }
//    PrintApp(device_app, false);
  }
  timer.EndScope();
//  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
  return Status::SUCCESS;
}

bool HasFixedView(const std::vector<Z3View>& z3_views_device) {
  for (const auto& view : z3_views_device) {
    if (view.pos == 0) continue;

    if (view.HasFixedPosition()) {
      return true;
    }
  }
  return false;
}

Status FullSynthesis::GetSatConstraintsSingleQuery(
    App& app,
    const OrientationContainer<AttrScorer>& scorers,
    std::vector<App>& device_apps,
    Timer& timer,
    OrientationContainer<CandidateConstraints>& candidates_all,
    BlockingConstraintsHelper* blocking_constraints) const {
  timer.StartScope("add_constraints");

  context c;
  solver s(c);

  std::vector<OrientationContainer<std::vector<Z3View>>> z3_views_devices_all;
  for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
    const App& device_app = device_apps[device_id];
    z3_views_devices_all.emplace_back(
        Z3View::ConvertViews(device_app.GetViews(), Orientation::HORIZONTAL, c, device_id + 1),
        Z3View::ConvertViews(device_app.GetViews(), Orientation::VERTICAL, c, device_id + 1)
    );
  }

  OrientationContainer<std::vector<Z3View>> z3_views_all(
      Z3View::ConvertViews(app.GetViews(), Orientation::HORIZONTAL, c),
      Z3View::ConvertViews(app.GetViews(), Orientation::VERTICAL, c)
  );


  AddConstraintsSingleQuery(app, device_apps, z3_views_devices_all, z3_views_all, candidates_all, false, s);
  if (blocking_constraints != nullptr) {
    blocking_constraints->AddBlockingConstraints(z3_views_devices_all, s);
  }

  timer.EndScope();
  timer.StartScope("solving");

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
  p.set(":unsat-core", true);
  s.set(p);


//  LOG(INFO) << s;

  Timer check_timer;
  check_timer.Start();
  check_result res = s.check(GetAssumptions(c, z3_views_all));

//  LOG(INFO) << "check_sat: " << res;
  int num_tries = 0;
  while (res == check_result::unsat) {

    num_tries++;
    if (num_tries > 100) break;
    if (check_timer.GetMilliSeconds() > timeout) {
      return Status::TIMEOUT;
//      break;
    }
//    LOG(INFO) << "Adding More Constraints: " << num_tries;
//    LOG(INFO) << "Solving...";
    timer.EndScope();
    timer.StartScope("additional_constraints");


    for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
      auto& candidates = candidates_all[orientation];
      auto& z3_views = z3_views_all[orientation];

      if (num_tries % 20 == 0) {
        for (Z3View& view : z3_views) {
          candidates.IncreaseRank(view, 5);
          view.IncSatisfiedId();
        }
      } else {
        for (Z3View *view : GetUnsatViews(s, z3_views)) {
          candidates.IncreaseRank(*view, 10);
          view->IncSatisfiedId();
        }
      }
//      candidates.DumpConstraintCounts();

      AddSynAttributes(s, z3_views, orientation, candidates, false);
      for (auto& z3_views_device : z3_views_devices_all) {
        AddGenAttributes(s, z3_views_device[orientation], app, orientation, candidates);
      }
      candidates.FinishAdding();

      FinishedAddingConstraints(s, z3_views);
    }

    timer.EndScope();
    timer.StartScope("solving");
    res = s.check(GetAssumptions(c, z3_views_all));
//    LOG(INFO) << "check_sat: " << res;
  }

  timer.EndScope();

//  LOG(INFO) << "Num tries: " << num_tries << ": res:" << res;
  if (res != check_result::sat) {
    if (check_timer.GetMilliSeconds() > timeout) {
      return Status::TIMEOUT;
    }
    if (res == check_result::unsat) {
      return Status::UNSAT;
    }
    CHECK(res == check_result::unknown);
    return Status::UNKNOWN;
  }

//  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
//    LOG(INFO) << "Satisfiable with candidates: " << JoinInts(candidates_all[orientation].constraints_max_rank, ',');
//  }

  return Status::SUCCESS;
}

std::pair<Status, CandidateConstraints> FullSynthesis::GetSatConstraints(
    App& app,
    const Orientation& orientation,
    const AttrScorer* scorer,
    const Device& ref_device,
    std::vector<App>& device_apps,
    Timer& timer,
    bool user_input,
    bool robust) const {
  timer.StartScope("add_constraints");
  LOG(INFO) << "Syn: " << orientation;
//  LOG(INFO) << "Initialize Constraints...";
  context c;
  solver s(c);

  std::vector<std::vector<Z3View>> z3_views_devices;
  // single device specification

  std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

  //creates
  CandidateConstraints candidates(scorer, z3_views);
  //5 constraints are allowed
  candidates.IncreaseRank(5);

  AddPositionConstraints(s, z3_views, true);
  AddAnchorConstraints(s, z3_views);
  AddSynAttributes(s, z3_views, orientation, candidates);
  //exactly one constraint will be matched
  FinishedAddingConstraints(s, z3_views);
  candidates.DumpConstraintCounts();
  // multi device constraints
  {

    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      App& device_app = device_apps[device_id];

      z3_views_devices.emplace_back(Z3View::ConvertViews(device_app.GetViews(), orientation, c, device_id + 1));
      std::vector<Z3View>& z3_views_device = z3_views_devices[z3_views_devices.size() - 1];

      // Set fixed size of the root layout
      s.add(z3_views_device[0].position_start_v == z3_views_device[0].start);
      s.add(z3_views_device[0].position_end_v == z3_views_device[0].end);

      if (user_input) {
        //if position is fixed/known -> set it
        for (size_t i = 1; i < z3_views.size(); i++) {
          const Z3View& view = z3_views_device[i];
          if (view.HasFixedPosition()) {
            s.add(view.position_start_v == view.start);
            s.add(view.position_end_v == view.end);
//            LOG(INFO) << "User Feedback device(" << device_id << "), view(" << view.pos << ") = [" << view.start << ", " << view.end << "]";
          }
        }
      }

      if (robust) {
        for (size_t i = 1; i < z3_views.size(); i++) {
          if (z3_views[i].HasFixedPosition()) continue;
          if (z3_views[i].end - z3_views[i].start > 0) {
            s.add(
                (z3_views_device[i].position_end_v - z3_views_device[i].position_start_v >
                 (z3_views[i].end - z3_views[i].start) / 2) &&
                (z3_views_device[i].position_end_v - z3_views_device[i].position_start_v <
                 (z3_views[i].end - z3_views[i].start) * 2)
            );
          }
        }

        AssertNotOutOfBounds(s, z3_views_device);

        AssertKeepsIntersection(s, app, z3_views, z3_views_device);
        AssertKeepsCentering(s, app, z3_views, z3_views_device);
        AssertKeepsMargins(s, app, z3_views, z3_views_device);
        if (orientation == Orientation::HORIZONTAL) {
          AssertKeepsSizeRatio(s, app, z3_views, z3_views_device, device_app);
        }
      }

      AddGenAttributes(s, z3_views_device, app, orientation, candidates);
    }
  }

  candidates.FinishAdding();

  timer.EndScope();
  timer.StartScope("solving");

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
  p.set(":unsat-core", true);
  s.set(p);

//  LOG(INFO) << s;

  Timer check_timer;
  check_timer.Start();
  check_result res = s.check(GetAssumptions(c, z3_views));

  //LOG(INFO) << "larissa whole fromula" << s;
  LOG(INFO) << "check_sat: " << res;
  int num_tries = 0;
  while (res == check_result::unsat) {

    num_tries++;
    if (num_tries > 50) break;
    if (check_timer.GetMilliSeconds() > timeout) {
      return std::make_pair(Status::TIMEOUT, candidates);
//      break;
    }
    LOG(INFO) << "Adding More Constraints: " << num_tries;
    LOG(INFO) << "Solving...";
    timer.EndScope();
    timer.StartScope("additional_constraints");

    for (Z3View* view : GetUnsatViews(s, z3_views)) {
      candidates.IncreaseRank(*view, 10);
      view->IncSatisfiedId();
    }

    candidates.DumpConstraintCounts();

    AddSynAttributes(s, z3_views, orientation, candidates, false);
    for (std::vector<Z3View>& z3_views_device : z3_views_devices) {
      AddGenAttributes(s, z3_views_device, app, orientation, candidates);
    }
    candidates.FinishAdding();

    FinishedAddingConstraints(s, z3_views);
    timer.EndScope();
    timer.StartScope("solving");
    res = s.check(GetAssumptions(c, z3_views));
    LOG(INFO) << "check_sat: " << res;
  }

  timer.EndScope();

  LOG(INFO) << "Num tries: " << num_tries << ": res:" << res;
  if (res != check_result::sat) {
    if (check_timer.GetMilliSeconds() > timeout) {
      return std::make_pair(Status::TIMEOUT, candidates);
    }
    if (res == check_result::unsat) {
      return std::make_pair(Status::UNSAT, candidates);
    }
    CHECK(res == check_result::unknown);
    return std::make_pair(Status::UNKNOWN, candidates);
  }

  LOG(INFO) << "Satisfiable with candidates: " << JoinInts(candidates.constraints_max_rank, ',');

  return std::make_pair(Status::SUCCESS, candidates);
}


std::pair<check_result, std::vector<Z3View*> > FullSynthesis::checkSatIntermediate(solver& s, std::vector<Z3View>& z3_views) const{
	  context c1;

	  std::stringstream buffer;
	  buffer << s;
	  std::string old = buffer.str();
	  solver s1(c1);
	  unsigned timeout = 60000u;
	  params p(c1);
	  p.set(":timeout", timeout);
	  p.set(":unsat-core", true);
	  s1.set(p);
	  s1.from_string(old.c_str());

	  expr_vector assumptions(c1);
	    for (Z3View& view : z3_views) {
	      if (view.pos == 0) continue;
	      //need to pass over the context here
	      assumptions.push_back(view.GetConstraintsSatisfied(c1));
	    }

	  check_result res = s1.check(assumptions);

	  std::vector<Z3View*> z3_unsat_views;
	  expr_vector core = s1.unsat_core();
	   for (Z3View& view : z3_views) {
	      if (view.pos == 0) continue;

	      std::string name = view.GetConstraintsSatisfied(c1).to_string();
	      bool sat = false;
	      for (unsigned i = 0; i < core.size(); i++) {
	        if (core[i].to_string() == name) {
	          sat = true;
	          break;
	        }
	      }
	      if (sat) {
	    	  z3_unsat_views.push_back(&view);
	      }
	    }
	  return make_pair(res, z3_unsat_views);
}

std::pair<Status, CandidateConstraints> FullSynthesis::GetSatConstraintsOracle(
    App& app,
    const Orientation& orientation,
    const AttrScorer* scorer,
    const Device& ref_device,
    const std::vector<App>& device_apps,
    Timer& timer,
	const std::vector<App>& blockedApps
	) const {
  timer.StartScope("add_constraints");
  LOG(INFO) << "Syn: " << orientation;
//  LOG(INFO) << "Initialize Constraints...";

  context c;
  solver s(c);

  std::vector<std::vector<Z3View>> z3_views_devices;
  std::vector<std::vector<Z3View>> z3_views_blocked;

  std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

  CandidateConstraints candidates(scorer, z3_views);
  //candidates.SetProhibitedConstraintMap(constraintMap);

  //5 constraints are allowed
  candidates.IncreaseRank(5);

  // single device specification
  AddPositionConstraints(s, z3_views, true);
  AddAnchorConstraints(s, z3_views);
  AddSynAttributes(s, z3_views, orientation, candidates);

  //exactly one constraint will be matched
  FinishedAddingConstraints(s, z3_views);
  candidates.DumpConstraintCounts();


  // multiple inputs
  {

    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      const App& device_app = device_apps[device_id];
//      LOG(INFO) << "\tdevice: " << device_app.views[0];

      //TODO: Check for screen with same dimensions

      z3_views_devices.emplace_back(Z3View::ConvertViews(device_app.GetViews(), orientation, c, device_id + 1));
      std::vector<Z3View>& z3_views_device = z3_views_devices[z3_views_devices.size() - 1];

      if(z3_views_device.size() < z3_views.size()){
    	  LOG(INFO) << z3_views_device.size() << " " << z3_views.size();
    	  LOG(FATAL) << "Fatal error (later access)";
      }

        if (!HasFixedView(z3_views_device)) {
          LOG(INFO) << "Error " << device_id << " with no user constraints";
          continue;
        }


        for (size_t i = 1; i < z3_views.size(); i++) {
          const Z3View& view = z3_views_device[i];
          if (view.HasFixedPosition()) {
            s.add(view.position_start_v == view.start);
            s.add(view.position_end_v == view.end);
            LOG(INFO) << "User Feedback device(" << device_id << "), view(" << view.pos << ") = [" << view.start << ", " << view.end << "]";
          }
        }
          // Set fixed size of the root layout
          s.add(z3_views_device[0].position_start_v == z3_views_device[0].start);
          s.add(z3_views_device[0].position_end_v == z3_views_device[0].end);

          AddGenAttributes(s, z3_views_device, app, orientation, candidates);
        }
  }


  for(const App& blockedApp: blockedApps){
	    z3_views_blocked.emplace_back(Z3View::ConvertViews(blockedApp.GetViews(), orientation, c, 4444));
      std::vector<Z3View>& z3_views_blo = z3_views_blocked[z3_views_blocked.size() - 1];
      AddGenAttributes(s, z3_views_blo, blockedApp, orientation, candidates);
	    addBlockingConstraints(s, z3_views_blo, c);
  }

  candidates.FinishAdding();

  timer.EndScope();
  timer.StartScope("solving");

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
  p.set(":unsat-core", true);
  s.set(p);

//  LOG(INFO) << s;

  Timer check_timer;
  check_timer.Start();


  check_result res = s.check(GetAssumptions(c, z3_views));

//  LOG(INFO) << "check_sat: " << res;

  int num_tries = 0;
  while (res == check_result::unsat) {

    num_tries++;
    if (num_tries > 50) break;
    if (check_timer.GetMilliSeconds() > timeout) {
      return std::make_pair(Status::TIMEOUT, candidates);
    }
    timer.EndScope();
    timer.StartScope("additional_constraints");


    for (Z3View* view : GetUnsatViews(s, z3_views)){
      candidates.IncreaseRank(*view, 10);
      view->IncSatisfiedId();
    }

//    candidates.DumpConstraintCounts();

    AddSynAttributes(s, z3_views, orientation, candidates);
    for (std::vector<Z3View>& z3_views_device : z3_views_devices) {
      if (!HasFixedView(z3_views_device)) {
        continue;
      }
      AddGenAttributes(s, z3_views_device, app, orientation, candidates);
    }
    for (std::vector<Z3View>& z3_views_blo : z3_views_blocked) {
      if (!HasFixedView(z3_views_blo)) {
        continue;
      }
      AddGenAttributes(s, z3_views_blo, app, orientation, candidates);
    }

    candidates.FinishAdding();

    FinishedAddingConstraints(s, z3_views);
    timer.EndScope();
    timer.StartScope("solving");

    res = s.check(GetAssumptions(c, z3_views));
    LOG(INFO) << "check_sat: " << res;
  }

  timer.EndScope();

//  LOG(INFO) << "Num tries: " << num_tries << ": res:" << res;
  if (res != check_result::sat) {
    if (check_timer.GetMilliSeconds() > timeout) {
      return std::make_pair(Status::TIMEOUT, candidates);
    }
    if (res == check_result::unsat) {
      return std::make_pair(Status::UNSAT, candidates);
    }
    CHECK(res == check_result::unknown);
    return std::make_pair(Status::UNKNOWN, candidates);
  }

  LOG(INFO) << "Satisfiable with candidates: " << JoinInts(candidates.constraints_max_rank, ',');
  candidates.DumpConstraintCounts();

  return std::make_pair(Status::SUCCESS, candidates);
}


Status FullSynthesis::SynthesizeMultiDeviceProbSingleQuery(
    App& app,
    const OrientationContainer<AttrScorer>& scorers,
    std::vector<App>& device_apps,
    Timer& timer,
    bool opt,
    BlockingConstraintsHelper* blocking_constraints) const {

  timer.StartScope("init");
  context c;

  OrientationContainer<std::vector<Z3View>> z3_views_all(
      Z3View::ConvertViews(app.GetViews(), Orientation::HORIZONTAL, c),
      Z3View::ConvertViews(app.GetViews(), Orientation::VERTICAL, c)
  );

  OrientationContainer<CandidateConstraints> candidates_all(
      CandidateConstraints(
          &scorers[Orientation::HORIZONTAL],
          z3_views_all[Orientation::HORIZONTAL]),
      CandidateConstraints(
          &scorers[Orientation::VERTICAL],
          z3_views_all[Orientation::VERTICAL])
  );

  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    auto &candidates = candidates_all[orientation];
    candidates.IncreaseRank(5);
  }

  timer.EndScope();

  Status r = GetSatConstraintsSingleQuery(app, scorers, device_apps, timer, candidates_all, blocking_constraints);
  if (r != Status::SUCCESS) {
    return r;
  }

  Status res = SynthesizeMultiDeviceProbSingleQueryInner(app, scorers, device_apps, timer, opt, c, candidates_all, blocking_constraints);
  if (!opt || res == Status::SUCCESS) {
    return res;
  }
  return SynthesizeMultiDeviceProbSingleQueryInner(app, scorers, device_apps, timer, false, c, candidates_all, blocking_constraints);
}

Status FullSynthesis::SynthesizeMultiDeviceProbSingleQueryInner(
    App& app,
    const OrientationContainer<AttrScorer>& scorers,
    std::vector<App>& device_apps,
    Timer& timer,
    bool opt,
    context& c,
    OrientationContainer<CandidateConstraints>& candidates_all,
    BlockingConstraintsHelper* blocking_constraints) const {

  std::vector<OrientationContainer<std::vector<Z3View>>> z3_views_devices_all;
  for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
    const App& device_app = device_apps[device_id];
    z3_views_devices_all.emplace_back(
        Z3View::ConvertViews(device_app.GetViews(), Orientation::HORIZONTAL, c, device_id + 1),
        Z3View::ConvertViews(device_app.GetViews(), Orientation::VERTICAL, c, device_id + 1)
    );
  }

  OrientationContainer<std::vector<Z3View>> z3_views_all(
      Z3View::ConvertViews(app.GetViews(), Orientation::HORIZONTAL, c),
      Z3View::ConvertViews(app.GetViews(), Orientation::VERTICAL, c)
  );

  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    candidates_all[orientation].ResetAdding();
  }

  timer.StartScope("add_constraints");
  optimize s(c);
  AddConstraintsSingleQuery(app, device_apps, z3_views_devices_all, z3_views_all, candidates_all, opt, s);
  if (blocking_constraints != nullptr) {
    blocking_constraints->AddBlockingConstraints(z3_views_devices_all, s);
  }
  timer.EndScope();

  timer.StartScope("solving");
  unsigned timeout = (opt) ? 20000u : 60000u;
  params p(c);
  p.set(":timeout", timeout);
  s.set(p);

  expr_vector assumptions = GetAssumptions(c, z3_views_all);
  for (size_t i = 0; i < assumptions.size(); i++) {
    s.add(assumptions[i]);
  }

  Timer check_timer;
  check_timer.Start();
  if (opt) {
    expr cost = c.real_val("0");
    for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
      for (Z3View &view : z3_views_all[orientation]) {
        if (view.pos == 0) continue;
        cost = cost + view.GetCostExpr();
      }
    }
    s.maximize(cost);
  }
  check_result res = s.check();
  timer.EndScope();

  if (res != check_result::sat) {
    LOG(INFO) << res << " for:";
    for (size_t i = 0; i < app.GetViews().size(); i++) {
      LOG(INFO) << '\t' << app.GetViews()[i];
    }

    if (check_timer.GetMilliSeconds() > timeout) {
      return Status::TIMEOUT;
    }

    LOG(INFO) << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nGot result: " << res << " but expected sat!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
    if (res == check_result::unsat) {
      return Status::UNSAT;
    }
    CHECK(res == check_result::unknown);
    return Status::UNKNOWN;
  }

  model m = s.get_model();
  timer.StartScope("generating_output");
  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    for (Z3View &view : z3_views_all[orientation]) {
      if (view.pos == 0) continue;

      view.AssignModel(m, orientation, app.GetViews());
    }
  }

  for (const auto& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      App &device_app = device_apps[device_id];
      std::vector<Z3View> &z3_views_device = z3_views_devices_all[device_id][orientation];

      CHECK_EQ(device_app.GetViews().size(), z3_views_device.size());
      for (size_t view_id = 1; view_id < z3_views_device.size(); view_id++) {
        Z3View &z3_view = z3_views_device[view_id];
        View &view = device_app.GetViews()[view_id];
        if (view.HasFixedPosition()) continue;

        z3_view.AssignPosition(m);
        if (orientation == Orientation::HORIZONTAL) {
          view.xleft = z3_view.start;
          view.xright = z3_view.end;
        } else {
          view.ytop = z3_view.start;
          view.ybottom = z3_view.end;
        }
      }
    }
  }

  timer.EndScope();
  return Status::SUCCESS;
}

Status FullSynthesis::SynthesizeMultiDeviceProb(
    App& app,
    const Orientation& orientation,
    const AttrScorer* scorer,
    const Device& ref_device,
    std::vector<App>& device_apps,
    Timer& timer,
    bool user_input,
    bool robust,
    bool opt) const {

  timer.StartScope("add_constraints");
  LOG(INFO) << "Syn: " << orientation;
//  LOG(INFO) << "Initialize Constraints...";
  context c;
  optimize s(c);

  std::vector<std::vector<Z3View>> z3_views_devices;
  // single device specification

  std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

  std::pair<Status, CandidateConstraints> r = GetSatConstraints(app, orientation, scorer, ref_device, device_apps, timer, user_input, robust);
  if (r.first != Status::SUCCESS) {
    return r.first;
  }
  CandidateConstraints& candidates = r.second;
  candidates.ResetAdding();

  AddPositionConstraints(s, z3_views, true);
  AddAnchorConstraints(s, z3_views);

  AddSynAttributes(s, z3_views, orientation, candidates, opt);
  FinishedAddingConstraints(s, z3_views);

  // multi device constraints
  {

    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      App& device_app = device_apps[device_id];

      z3_views_devices.emplace_back(Z3View::ConvertViews(device_app.GetViews(), orientation, c, device_id + 1));
      std::vector<Z3View>& z3_views_device = z3_views_devices[z3_views_devices.size() - 1];

      // Set fixed size of the root layout
      s.add(z3_views_device[0].position_start_v == z3_views_device[0].start);
      s.add(z3_views_device[0].position_end_v == z3_views_device[0].end);

      if (user_input) {
        //if position is fixed/known -> set it
        for (size_t i = 1; i < z3_views.size(); i++) {
          const Z3View& view = z3_views_device[i];
          if (view.HasFixedPosition()) {
            s.add(view.position_start_v == view.start);
            s.add(view.position_end_v == view.end);
//            LOG(INFO) << "User Feedback device(" << device_id << "), view(" << view.pos << ") = [" << view.start << ", " << view.end << "]";
          }
        }
      }

      if (robust) {
        for (size_t i = 1; i < z3_views.size(); i++) {
          if (z3_views[i].HasFixedPosition()) continue;
          if (z3_views[i].end - z3_views[i].start > 0) {
            s.add(
                (z3_views_device[i].position_end_v - z3_views_device[i].position_start_v >
                 (z3_views[i].end - z3_views[i].start) / 2) &&
                (z3_views_device[i].position_end_v - z3_views_device[i].position_start_v <
                 (z3_views[i].end - z3_views[i].start) * 2)
            );
          }
        }

        AssertNotOutOfBounds(s, z3_views_device);

        AssertKeepsIntersection(s, app, z3_views, z3_views_device);
        AssertKeepsCentering(s, app, z3_views, z3_views_device);
        AssertKeepsMargins(s, app, z3_views, z3_views_device);
        if (orientation == Orientation::HORIZONTAL) {
          AssertKeepsSizeRatio(s, app, z3_views, z3_views_device, device_app);
        }
      }

      AddGenAttributes(s, z3_views_device, app, orientation, candidates);

    }
  }

  timer.EndScope();
  timer.StartScope("solving");

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
//  p.set(":model", true);
//  p.set(":unsat-core", true);
  s.set(p);

  expr_vector assumptions = GetAssumptions(c, z3_views);
  for (size_t i = 0; i < assumptions.size(); i++) {
    s.add(assumptions[i]);
  }

  Timer check_timer;
  check_timer.Start();
  if (opt) {
//    expr x = c.real_const("x");
    expr cost = c.real_val("0");
    for (Z3View &view : z3_views) {
      if (view.pos == 0) continue;
      cost = cost + view.GetCostExpr();
    }
//    LOG(INFO) << cost;
//    s.add(x == cost);
    s.maximize(cost);
  }

  check_result res = s.check();


  timer.EndScope();

  if (res != check_result::sat) {
    LOG(INFO) << res << " for:";
    for (size_t i = 0; i < app.GetViews().size(); i++) {
      LOG(INFO) << '\t' << app.GetViews()[i];
    }

    if (check_timer.GetMilliSeconds() > timeout) {
      return Status::TIMEOUT;
    }

    LOG(INFO) << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nGot result: " << res << " but expected sat!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
    if (res == check_result::unsat) {
      return Status::UNSAT;
    }
    CHECK(res == check_result::unknown);
    return Status::UNKNOWN;
  }

  model m = s.get_model();
  timer.StartScope("generating_output");
  for (Z3View& view : z3_views) {
    if (view.pos == 0) continue;

    view.AssignModel(m, orientation, app.GetViews());

  }

//  PrintApp(app, false);

  if (!user_input) {
    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      App &device_app = device_apps[device_id];
      std::vector<Z3View> &z3_views_device = z3_views_devices[device_id];

      CHECK_EQ(device_app.GetViews().size(), z3_views_device.size());
      for (size_t view_id = 1; view_id < z3_views_device.size(); view_id++) {

        Z3View &z3_view = z3_views_device[view_id];
        View &view = device_app.GetViews()[view_id];
        z3_view.AssignPosition(m);
        if (orientation == Orientation::HORIZONTAL) {
          view.xleft = z3_view.start;
          view.xright = z3_view.end;
        } else {
          view.ytop = z3_view.start;
          view.ybottom = z3_view.end;
        }
      }
//    PrintApp(device_app, false);
    }
  }
  timer.EndScope();
  return Status::SUCCESS;
}



static std::atomic<int> fatalExpectedSatGotUnsat(0);
//ContraintMap contains the selected constraints
std::pair<Status, ConstraintMap> FullSynthesis::SynthesizeDeviceProbOracle(
    App& app,
    const Orientation& orientation,
    const AttrScorer* scorer,
    const Device& ref_device,
    const std::vector<App>& device_apps, //app where we know certain view positions because of CEGIS loop
	std::vector<App>& target_apps,
    Timer& timer,
    bool opt,
	const std::vector<App>& blockedApps
	) const {


  timer.StartScope("add_constraints");

  ConstraintMap selectedConstraints;

  LOG(INFO) << "Syn: " << orientation;
//  LOG(INFO) << "Initialize Constraints...";
  context c;
  optimize s(c);

  std::vector<std::vector<Z3View>> z3_views_devices;
  std::vector<std::vector<Z3View>> z3_views_target_devices;

  // single device specification

  std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

  LOG(INFO) << "before sat" << blockedApps.size() << " " << s;

  std::pair<Status, CandidateConstraints> r = GetSatConstraintsOracle(app, orientation, scorer, ref_device, device_apps, timer, blockedApps);
  if (r.first != Status::SUCCESS) {
	  return std::make_pair(r.first, selectedConstraints);
  }

  CandidateConstraints& candidates = r.second;
  LOG(INFO) << "SynthesizeDeviceProbOracle, candidates";
  candidates.DumpConstraintCounts();
  candidates.ResetAdding();

  AddPositionConstraints(s, z3_views, true);
  AddAnchorConstraints(s, z3_views);



//  AddSynConstraints(s, z3_views, orientation, candidates);
  AddSynAttributes(s, z3_views, orientation, candidates, opt);
  FinishedAddingConstraints(s, z3_views);

  // constraints from CEGIS loop
  {
    for (size_t device_id = 0; device_id < device_apps.size(); device_id++) {
      const App& device_app = device_apps[device_id];

      z3_views_devices.emplace_back(Z3View::ConvertViews(device_app.GetViews(), orientation, c, device_id + 1));
      std::vector<Z3View>& z3_views_device = z3_views_devices[z3_views_devices.size() - 1];

      if(z3_views_device.size() < z3_views.size()){
    	  LOG(FATAL) << "Fatal error (later access)";
      }

        if (!HasFixedView(z3_views_device)) {
          LOG(INFO) << "Error " << device_id << " with no constraints";
          continue;
        }

        for (size_t i = 1; i < z3_views.size(); i++) {
          const Z3View& view = z3_views_device[i];
          if (view.HasFixedPosition()) {
            s.add(view.position_start_v == view.start);
            s.add(view.position_end_v == view.end);
            LOG(INFO) << "User Feedback device(" << device_id << "), view(" << view.pos << ") = [" << view.start << ", " << view.end << "]";
          }
        }

          // Set fixed size of the root layout
          s.add(z3_views_device[0].position_start_v == z3_views_device[0].start);
          s.add(z3_views_device[0].position_end_v == z3_views_device[0].end);

          AddGenAttributes(s, z3_views_device, app, orientation, candidates);
    }
  }


  for(const App& blockedApp: blockedApps){
	  std::vector<Z3View> z3_views_blo = Z3View::ConvertViews(blockedApp.GetViews(), orientation, c, 4444);
      AddGenAttributes(s, z3_views_blo, blockedApp, orientation, candidates);
	  addBlockingConstraints(s, z3_views_blo, c);
  }

   //To get the view coordinates back for other devices
  for (size_t device_id = 0; device_id < target_apps.size(); device_id++) {
    App& target_app = target_apps[device_id];
    z3_views_target_devices.emplace_back(Z3View::ConvertViews(target_app.GetViews(), orientation, c, 404 + device_id));
    std::vector<Z3View>& z3_views_target_device = z3_views_target_devices[z3_views_target_devices.size() - 1];
    s.add(z3_views_target_device[0].position_start_v == z3_views_target_device[0].start);
    s.add(z3_views_target_device[0].position_end_v == z3_views_target_device[0].end);
    AddGenAttributes(s, z3_views_target_device, app, orientation, candidates);
  }



  timer.EndScope();
  timer.StartScope("solving");

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
//  p.set(":model", true);
//  p.set(":unsat-core", true);
  s.set(p);

  expr_vector assumptions = GetAssumptions(c, z3_views);
  for (size_t i = 0; i < assumptions.size(); i++) {
    s.add(assumptions[i]);
  }


  Timer check_timer;
  check_timer.Start();

  if (opt) {
    expr cost = c.real_val("0");
    for (Z3View &view : z3_views) {
      if (view.pos == 0) continue;
      cost = cost + view.GetCostExpr();
    }

    s.maximize(cost);
  }

  check_result res = s.check();

  timer.EndScope();

  if (res != check_result::sat) {
    LOG(INFO) << res << " for:";
    for (size_t i = 0; i < app.GetViews().size(); i++) {
      LOG(INFO) << '\t' << app.GetViews()[i];
    }

    if (check_timer.GetMilliSeconds() > timeout) {
      return std::make_pair(Status::TIMEOUT, selectedConstraints);
    }
    LOG(INFO) << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\nGot result: " << res << " but expected sat!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
    fatalExpectedSatGotUnsat++;
    LOG(INFO) << "fatalExpectedSatGotUnsat" << fatalExpectedSatGotUnsat;
    if (res == check_result::unsat) {
      return std::make_pair(Status::UNSAT, selectedConstraints);
    }
    CHECK(res == check_result::unknown);
    return std::make_pair(Status::UNKNOWN, selectedConstraints);
  }

  model m = s.get_model();
  timer.StartScope("generating_output");
  for (Z3View& view : z3_views) {
    if (view.pos == 0) continue;
    if(selectedConstraints.find(view.pos) != selectedConstraints.end()){
    	LOG(FATAL) << "The same view has more than one selected constraint for one orientation.";
    }
    selectedConstraints[view.pos] = view.AssignModel(m, orientation, app.GetViews(), scorer);
  }

  for (size_t device_id = 0; device_id < target_apps.size(); device_id++) {
        App &target_app = target_apps[device_id];
        std::vector<Z3View> &z3_views_device = z3_views_target_devices[device_id];

        CHECK_EQ(target_app.GetViews().size(), z3_views_device.size());
        for (size_t view_id = 1; view_id < z3_views_device.size(); view_id++) {

          Z3View &z3_view = z3_views_device[view_id];
          View &view = target_app.GetViews()[view_id];
          z3_view.AssignPosition(m);
          if (orientation == Orientation::HORIZONTAL) {
            view.xleft = z3_view.start;
            view.xright = z3_view.end;
          } else {
            view.ytop = z3_view.start;
            view.ybottom = z3_view.end;
          }
        }
        //PrintApp(target_app, false);
      }
  timer.EndScope();

  return std::make_pair(Status::SUCCESS, selectedConstraints);
}

Status FullSynthesis::Synthesize(App& app, const Orientation& orientation) const {
  Timer timer;
  timer.Start();
  VLOG(2) << "Initialize Constraints...";
  context c;
  solver s(c);

  std::vector<Z3View> z3_views = Z3View::ConvertViews(app.GetViews(), orientation, c);

  AddPositionConstraints(s, z3_views);
  AddAnchorConstraints(s, z3_views);

  ForEachNonRootView(s, orientation, z3_views, AddFixedSizeRelationalConstraint<solver>,
                     [](const std::string& name, const Z3View& src){
                       return true;
                     });

  ForEachNonRootView(s, orientation, z3_views, AddFixedSizeCenteringConstraint<solver>,
                     [](const std::string &name, const Z3View &src) {
                       return true;
                     });

  ForEachNonRootView(s, orientation, z3_views, AddMatchConstraintCenteringConstraint<solver>,
                     [](const std::string& name, const Z3View& src){
                       return true;
                     });

  FinishedAddingConstraints(s, z3_views);


  VLOG(2) << "Done in " << (timer.Stop() / 1000) << "ms";
  VLOG(2) << "Solving...";
  timer.Start();

  unsigned timeout = 60000u;
  params p(c);
  p.set(":timeout", timeout);
  s.set(p);

  check_result res = s.check(GetAssumptions(c, z3_views));
  VLOG(2) << res;
  LOG(INFO) << res;

  LOG(INFO) << "Done in " << (timer.Stop() / 1000) << "ms";
  if (res != check_result::sat) {
    if (timer.GetMilliSeconds() > timeout) {
      return Status::TIMEOUT;
    }
    if (res == check_result::unsat) {
      return Status::UNSAT;
    }
    CHECK(res == check_result::unknown);
    return Status::UNKNOWN;
  }
  model m = s.get_model();
  VLOG(2) << "Generating Output...";
  timer.Start();


  for (Z3View& view : z3_views) {
    if (view.pos == 0) continue;

    view.AssignModel(m, orientation, app.GetViews());
  }
  VLOG(2) << "Done in " << (timer.Stop() / 1000) << "ms";
  return Status::SUCCESS;
}


void CandidateConstraints::IncreaseRank(int value) {
  for (int& entry : constraints_max_rank) {
    entry += value;
  }

}

void CandidateConstraints::IncreaseRank(const Z3View& view, int value) {
  constraints_max_rank[view.pos] += value;
//    LOG(INFO) << "ConstraintsMaxRank: " << JoinInts(constraints_max_rank, ',');
}

std::vector<const Attribute*> CandidateConstraints::GetAttributes(const Z3View& view) {
  std::vector<const Attribute*> res;

  // Add Anchor

  if (constraints_added[view.pos] == 0) {
    res.push_back(scorer->GetAttr(view.pos, anchors[view.pos]));
    constraints_selected[view.pos].insert(anchors[view.pos]);
  }

  for (int rank = constraints_added[view.pos]; rank < constraints_max_rank[view.pos]; rank++) {
    const Attribute* attr = scorer->GetAttr(view.pos, rank);
    if (attr == nullptr) break;
    if (!scorer->IsAllowed(attr) || IsProhibitedConstraint(attr)) continue;
    //was already added in the beginning
    if (rank == anchors[view.pos]) continue;

    res.push_back(attr);

    constraints_selected[view.pos].insert(rank);
  }

  return res;
}

bool CandidateConstraints::ShouldAdd(const Z3View& view, const std::string& constraint) const {
  if (scorer == nullptr) return true;
  if (constraints_added[view.pos] == constraints_max_rank[view.pos]) return false;
  std::pair<int, double> rank = scorer->GetRank(constraint, constraints_max_rank[view.pos]);
  return rank.first >= constraints_added[view.pos] && rank.first < constraints_max_rank[view.pos];
}
