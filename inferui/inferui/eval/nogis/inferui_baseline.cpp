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


PropertyStats SingleSyn(const DatasetIterators& it, bool opt) {
  auto synthesizer = GenSmtMultiDeviceProbOpt(opt);
  PropertyStats stats = it.ForEachDSPlusApp(DatasetType::TEST,
                                            [&](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id){
                                              // Everything here needs to be thread safe
                                              std::vector<Device> devices_syn;
                                              return synthesizer.Synthesize(std::move(app), ref_device, devices_syn);
                                            });
  return stats;
}

PropertyStats SingleSynOneQuery(const DatasetIterators& it, bool opt) {
  auto synthesizer = GenSmtMultiDeviceProbOpt(opt);
  PropertyStats stats = it.ForEachDSPlusApp(DatasetType::TEST,
                                            [&](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id){
                                              // Everything here needs to be thread safe
                                              std::vector<App> input_apps;
                                              for (const auto& dev_app : apps) {
                                                input_apps.push_back(EmptyApp(dev_app));
                                              }
                                              return synthesizer.SynthesizeMultipleAppsSingleQuery(std::move(app), input_apps);
                                            });
  return stats;
}

PropertyStats RobustSyn(const DatasetIterators& it, bool opt) {
  auto synthesizer = GenSmtMultiDeviceProbOpt(opt);
  PropertyStats stats = it.ForEachDSPlusApp(DatasetType::TEST,
                                            [&](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id){
                                              // Everything here needs to be thread safe
                                              return synthesizer.Synthesize(std::move(app), ref_device, devices);
                                            });
  return stats;
}

PropertyStats UserFeedbackSingleSyn(const DatasetIterators& it, bool opt) {
  auto base_synthesizer = GenSmtMultiDeviceProbOpt(opt);
  const auto cb = [&](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id){
    // Everything here needs to be thread safe
    std::vector<App> input_apps;
    for (const auto& dev_app : apps) {
      input_apps.emplace_back(dev_app);
    }
    return base_synthesizer.SynthesizeMultipleAppsSingleQuery(std::move(app), input_apps);
  };

  UserFeedbackSynthesis synthesis(std::move(cb));
  PropertyStats stats = it.ForEachDSPlusApp(DatasetType::TEST,
                                            [&](App app, const std::vector<App>& apps,
                                                const Device& ref_device, const std::vector<Device>& devices,
                                                int app_id) {
    return synthesis.Synthesize(app, apps, ref_device, devices, app_id);
  });

  stats.fixed_views = synthesis.fixed_views;
  stats.total_views = synthesis.total_views;
  LOG(INFO) << "Fixed Views: " << synthesis.fixed_views << "/" << synthesis.total_views;
  return stats;
}

PropertyStats UserFeedbackRobustSyn(const DatasetIterators& it, bool opt) {
  auto base_synthesizer = GenSmtMultiDeviceProbOpt(opt);
  const auto cb = [&](App app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id){
    // Everything here needs to be thread safe
    std::vector<App> input_apps;
    for (const auto& dev_app : apps) {
      input_apps.emplace_back(dev_app);
    }
    return base_synthesizer.Synthesize(std::move(app), ref_device, input_apps);
  };


  UserFeedbackSynthesis synthesis(std::move(cb));
  PropertyStats stats = it.ForEachDSPlusApp(DatasetType::TEST,
                                            [&](App app, const std::vector<App>& apps,
                                                const Device& ref_device, const std::vector<Device>& devices,
                                                int app_id) {
                                              return synthesis.Synthesize(app, apps, ref_device, devices, app_id);
                                            });

  stats.fixed_views = synthesis.fixed_views;
  stats.total_views = synthesis.total_views;
  LOG(INFO) << "Fixed Views: " << synthesis.fixed_views << "/" << synthesis.total_views;
  return stats;
}



int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_train_data != "data/github_top500_v2_full.proto") {
    LOG(INFO) << "Setting --train_data=data/github_top500_v2_full.proto instead of the user supplied value!";
    FLAGS_train_data = "data/github_top500_v2_full.proto";
  }
  if (FLAGS_scaling_factor != 2){
    LOG(INFO) << "Setting --scaling_factor=2 instead of the user supplied value!";
    FLAGS_scaling_factor = 2;
  }
  
  auto dataset_iterator = DatasetIterators();

  std::map<std::string, PropertyStats> results;
//  results["SingleSyn+Opt"] = SingleSyn(dataset_iterator, true);
//  results["SingleSyn"] = SingleSyn(dataset_iterator, false);

  results["SingleSynOneQuery+Opt"] = SingleSynOneQuery(dataset_iterator, true);
  results["SingleSynOneQuery"] = SingleSynOneQuery(dataset_iterator, false);

  results["RobustSyn+Opt"] = RobustSyn(dataset_iterator, true);
//  results["RobustSyn"] = RobustSyn(dataset_iterator, false);

//  results["UserFeedbackSingleSyn+Opt"] = UserFeedbackSingleSyn(dataset_iterator, true);
//  results["UserFeedbackRobustSyn+Opt"] = UserFeedbackRobustSyn(dataset_iterator, true);

  LOG(INFO) << "Results:";
  for (auto& it : results) {
    LOG(INFO) << "\t" << it.first;
    it.second.Dump();
  }

  return 0;
}

