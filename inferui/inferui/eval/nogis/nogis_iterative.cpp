/*
   Copyright 2019 Software Reliability Lab, ETH Zurich

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

#include <unordered_set>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <atomic>
#include "inferui/synthesis/z3inference.h"
#include "inferui/layout_solver/solver.h"
#include "inferui/eval/eval_util.h"
#include "inferui/eval/eval_app_util.h"
#include "inferui/model/util/util.h"
#include "inferui/datasets/dataset_util.h"
#include "base/range.h"

DEFINE_string(path, "data/neural_oracle/D_S+/data_post.json", "Path to directory with json files containing input specification");
DEFINE_string(target_path, "./testdata/", "Path to directory in which the resulting data is stored");
DEFINE_int32(max_candidates, 16, "Maximum Number of Candidate Layouts at each iteration");
DEFINE_bool(gen_data, false, "Evaluate if eval is true, otherwise generate data.");

static std::atomic<int> total(0), maximumIn(0), correctPresent(0), correctOneIsLowestRank(0);

SynResult iterativeSynthesis(GenSmtMultiDeviceProbOpt& synthesizer,
		std::string& oracle, std::string& dataset, bool generate_data, App app,
		const std::vector<App>& apps, const std::vector<Device>& devices,
		int app_id) {
	Solver solver;
	// TODO: pick for which resolution we want to generate the training dataset and which should be used as input
	std::vector<App> refinement_apps;
	//apps are empty during eval
	refinement_apps.push_back(EmptyApp(apps[0]));
	//refinement_apps.push_back(EmptyApp(apps[1]));

	bool found_correct_candidate = false;
	std::vector<double> scores;
	std::vector < std::vector < App >> all_candidate_device_apps;

	return synthesizer.SynthesizeMultipleAppsIterative(std::move(app),
			refinement_apps, FLAGS_max_candidates,
			[&apps, &found_correct_candidate, &app, &solver, &devices, &scores, &all_candidate_device_apps, &generate_data, &oracle, &dataset](int candidate_id, const App& candidate_app, const std::vector<App>& candidate_device_apps) {
				/* last view in candidate_device_apps is the candidate view that we are trying to predict at this iteration
				 * Each time this callback is invoked, the last view has a new position, different to all the previous ones
				 *
				 * return true: continues generating candidates until max_candidates or no more candidates exists
				 * return false: selects current candidate as the correct one and proceeds to generating candidates for next view
				 */

				//baseline experiment -> always take the first
				//return false;
				double score = 1.0;
				if(!generate_data) {
					score = askOracle(candidate_device_apps[0], solver, devices[0], oracle, dataset, "filename", apps[0], app);
				}
				scores.push_back(score);
				all_candidate_device_apps.push_back(candidate_device_apps);

				//return true -> continue searching
				//return false -> stop searching
				return true;

			},
			[&apps, &found_correct_candidate, &scores, &all_candidate_device_apps, &app, &app_id, &generate_data](int num_views, const App& candidate_app) {
				/*
				 * Called if the previous callback never returned false.
				 * In evaluation, we would pick here the argmax of all the candidates returned in the previous callback (for this iteration)
				 * In dataset generation we return the ground-truth
				 */

				if(!generate_data) {
					LOG(INFO) << "maximum scores: " << *std::max_element(scores.begin(), scores.end()) << " , " << std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
					int selectedIndex = std::distance(scores.begin(), std::max_element(scores.begin(), scores.end()));
					/*
					int lastView = all_candidate_device_apps[0][0].GetViews().size()-1;

					 for(unsigned i = 0; i < scores.size(); i++) {
						 bool match = ViewMatch(all_candidate_device_apps[i][0].GetViews()[lastView], apps[0].GetViews()[lastView]);
						 if(match){
							 found_correct_candidate = true;
						 }
						 if(match && scores[i] == *std::max_element(scores.begin(), scores.end())){
							 maximumIn++;
						 }
						 if(match && i == selectedIndex){
							 correctOneIsLowestRank++;
						 }
					 }
					 total++;
					 if(found_correct_candidate){
						 correctPresent++;
					 }
					 */

					std::vector<App> res = all_candidate_device_apps[selectedIndex];
					//std::vector<App> res = {KeepFirstNViews(apps[0], num_views)};
					return res;
				} else {
					//dataset generation
					std::vector<App> res = {KeepFirstNViews(apps[0], num_views)};
					//std::vector<App> res = {KeepFirstNViews(apps[0], num_views), KeepFirstNViews(apps[1], num_views)};
					writeAppData(FLAGS_target_path, std::to_string(app_id), all_candidate_device_apps, {apps[0]}, app);
					//writeData(std::to_string(_id), all_candidate_device_apps, apps, app);
					return res;
				}
			},
			[&found_correct_candidate, &all_candidate_device_apps, &scores](int num_views) {
				/*
				 * Called after a view position has been fixed (regardless of whether using first or second callback)
				 */
				all_candidate_device_apps.clear();
				scores.clear();
				found_correct_candidate = false;
			});
}

PropertyStats evaluatePlaystore(const DatasetIterators& it, bool opt,
		std::string oracle, std::string dataset, bool generate_data) {

	auto datasetType = DatasetType::ALL;

	if (!generate_data) {
		datasetType = DatasetType::TEST;
	}

	if (FLAGS_train_data != "data/github_top500_v2_full.proto") {
		LOG(INFO)
				<< "Setting --train_data=data/github_top500_v2_full.proto instead of the user supplied value!";
		FLAGS_train_data = "data/github_top500_v2_full.proto";
	}
	if (FLAGS_scaling_factor != 2) {
		LOG(INFO)
				<< "Setting --scaling_factor=2 instead of the user supplied value!";
		FLAGS_scaling_factor = 2;
	}

	auto synthesizer = GenSmtMultiDeviceProbOpt(opt);
	PropertyStats stats =
			it.ForEachPlaystoreApp(datasetType,
					[&synthesizer, &oracle, &dataset, &generate_data](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id) {
						// Everything here needs to be thread safe
						return iterativeSynthesis(synthesizer, oracle, dataset, generate_data, app, apps, devices, app_id);
					});
	return stats;
}

PropertyStats evaluateGithub(const DatasetIterators& it, bool opt,
		std::string oracle, std::string dataset, bool generate_data) {

	auto datasetType = DatasetType::ALL;

	if (!generate_data) {
		datasetType = DatasetType::TEST;
	}

	if (FLAGS_train_data != "data/constraint_layout_playstore_v2_full.proto") {
		LOG(INFO)
				<< "Setting --train_data=data/constraint_layout_playstore_v2_full.proto instead of the user supplied value!";
		FLAGS_train_data = "data/constraint_layout_playstore_v2_full.proto";
	}
	if (FLAGS_scaling_factor != 2) {
		LOG(INFO)
				<< "Setting --scaling_factor=2 instead of the user supplied value!";
		FLAGS_scaling_factor = 2;
	}
	auto synthesizer = GenSmtMultiDeviceProbOpt(opt);
	PropertyStats stats =
			it.ForEachGithubDPApp(datasetType,
					[&synthesizer, &oracle, &dataset, &generate_data](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id) {
						// Everything here needs to be thread safe
						return iterativeSynthesis(synthesizer, oracle, dataset, generate_data, app, apps, devices, app_id);
					});
	return stats;
}

PropertyStats evaluateDSPlus(const DatasetIterators& it, bool opt,
		std::string oracle, std::string dataset, bool generate_data) {
	auto datasetType = DatasetType::ALL;

	if (!generate_data) {
		datasetType = DatasetType::TEST;
	}

	if (FLAGS_train_data != "data/github_top500_v2_full.proto") {
		LOG(INFO)
				<< "Setting --train_data=data/github_top500_v2_full.proto instead of the user supplied value!";
		FLAGS_train_data = "data/github_top500_v2_full.proto";
	}
	if (FLAGS_scaling_factor != 2) {
		LOG(INFO)
				<< "Setting --scaling_factor=2 instead of the user supplied value!";
		FLAGS_scaling_factor = 2;
	}

	auto synthesizer = GenSmtMultiDeviceProbOpt(opt);
	PropertyStats stats =
			it.ForEachDSPlusApp(datasetType,
					[&synthesizer, &oracle, &dataset, &generate_data](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id) {
						// Everything here needs to be thread safe
						return iterativeSynthesis(synthesizer, oracle, dataset, generate_data, app, apps, devices, app_id);
					});
	return stats;
}

int main(int argc, char** argv) {
	google::InstallFailureSignalHandler();
	google::ParseCommandLineFlags(&argc, &argv, true);
	google::InitGoogleLogging(argv[0]);

	auto dataset_iterator = DatasetIterators();

	std::map<std::string, PropertyStats> results;

	//results["trest"] = evaluate(dataset_iterator, true, "MLP","paperds", pathForDataset("ds+"), false);

	auto models = {"MLP", "CNN", "doubleRNN", "ensembleRnnCnnBoth"};
	auto evalDatasets = {"ds+"}; //{ "dpp", "ds+", "dpg"};
	auto trainedDatasets = {"dsplus"}; //{ "ds" , "dsplus"};
	auto optModes = {"-"}; //{ "+OPT", "-" };

	for (auto optMode : optModes) {
		bool opt = (optMode == std::string("+OPT"));
		for (auto evalDataset : evalDatasets) {
			for (auto model : models) {
				for (auto dataset : trainedDatasets) {
					std::stringstream ss;
					ss << model << "-" << evalDataset << "-" << dataset
							<< optMode;
					PropertyStats res;
					if (evalDataset == std::string("ds+")) {
						res = evaluateDSPlus(dataset_iterator, opt, model,
								dataset, FLAGS_gen_data);
					} else if (evalDataset == std::string("dpp")) {
						res = evaluatePlaystore(dataset_iterator, opt, model,
								dataset, FLAGS_gen_data);
					} else if (evalDataset == std::string("dpg")) {
						res = evaluateGithub(dataset_iterator, opt, model,
								dataset, FLAGS_gen_data);
					}
					LOG(INFO) << "Intermediate result: " << ss.str();
					res.Dump();
					results[ss.str()] = res;
				}
			}
		}
	}
	LOG(INFO) << "Interesting value: total: " << total  << ", containedCorrect " <<  correctPresent << " , correctHadMaxScore: " << maximumIn << ", selectedCorrect: " << correctOneIsLowestRank;

	LOG(INFO) << "Results:";
	for (auto& it : results) {
		LOG(INFO) << "\t" << it.first;
		it.second.Dump();
	}

	return 0;
}
