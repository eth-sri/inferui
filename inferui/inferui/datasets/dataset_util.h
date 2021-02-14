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

#ifndef CC_SYNTHESIS_DATASET_UTIL_H
#define CC_SYNTHESIS_DATASET_UTIL_H

#include <map>
#include "inferui/model/util/util.h"
#include "inferui/model/model.h"
#include "inferui/model/syn_helper.h"
#include "inferui/synthesis/z3inference.h"
#include "inferui/layout_solver/solver.h"
#include "inferui/eval/eval_util.h"

DECLARE_bool(base_syn_fallback);

struct PropertyStats {
public:

  PropertyStats() : total(0), fully_correct(0), total_apps(0), success_apps(0), inconsistent_apps(0),
                    failed_syn_apps(0), unsat_apps(0), timeout_apps(0),
                    fixed_views(0), total_views(0) {
    values[Orientation::HORIZONTAL] = std::make_pair(0, 0);
    values[Orientation::VERTICAL] = std::make_pair(0, 0);
  }

  void Add(const Orientation& orientation, bool correct);

  void AddView(bool correct_horizontal, bool correct_vertical);

  void Dump() const;

private:
  std::map<Orientation, std::pair<int, int>> values;
  int total, fully_correct;

public:
  int total_apps, success_apps, inconsistent_apps, failed_syn_apps, unsat_apps, timeout_apps;
  int fixed_views, total_views;
};

bool ViewsInsideScreen(const App& app);

enum DatasetType {
  TRAIN = 0,
  VALID,
  TEST,
  ALL
};

class DatasetIterators {

public:

  PropertyStats ForEachApp(
      const std::string& path,
      const std::function<bool(const App&, int)>& contains_sample_cb,
      const std::function<SynResult(App&, const std::vector<App>&, const Device&, const std::vector<Device>&, int app_id)>& cb,
      int num_samples = -1) const;

  PropertyStats ForEachApp(
      const std::string& path,
      const std::function<SynResult(App&, const std::vector<App>&, const Device&, const std::vector<Device>&, int app_id)>& cb,
      int num_samples = -1) const {
    return ForEachApp(path,
                      [](const App& app, int app_idx) {
                        return (app.GetViews().size() <= 30);
                      }, cb, num_samples);
  }

  PropertyStats ForEachDSPlusApp(
      DatasetType type,
      const std::function<SynResult(App, const std::vector<App>& apps, const Device&, const std::vector<Device>&, int app_id)>& cb,
      int num_samples = -1) const {
    return ForEachApp("data/neural_oracle/D_S+/data_post.json",
        [&type](const App& app, int app_idx) {
          if (app.GetViews().size() > 30) {
            return false;
          }
          // check if the app is in the correct dataset
          switch (type) {
            case DatasetType::ALL:
              return true;
            case DatasetType::TRAIN:
              return false;
            case DatasetType::VALID:
              return app_idx > 328 && app_idx <= 579;
            case DatasetType::TEST:
              return app_idx <= 328;
          }
          CHECK(false) << "switch does not cover all the dataset types";
    }, [&cb](App& app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id) {
          CHECK_EQ(ref_device.width, 1440);
          CHECK_EQ(ref_device.height, 2560);
          // take the smaller device
          std::vector<Device> gen_devices = {devices[0]};
          CHECK_EQ(gen_devices[0].width, 1400);
          CHECK_EQ(gen_devices[0].height, 2520);

          std::vector<App> gen_apps = {apps[0]};
          // ensure that the ground-truth is not used during test
//          std::vector<App> empty_apps;
//          for(auto& t_app: apps){
//        	  empty_apps.push_back(EmptyApp(t_app));
//          }
//          return cb(app, (type == DatasetType::TEST) ? empty_apps : apps, ref_device, gen_devices, app_id);
          return cb(app, gen_apps, ref_device, gen_devices, app_id);
    }, num_samples);
  }

  PropertyStats ForEachDPApp(
		const std::string path,
        DatasetType type,
        const std::function<SynResult(App, const std::vector<App>& apps, const Device&, const std::vector<Device>&, int app_id)>& cb) const {
      return ForEachApp(path,
          [&type](const App& app, int app_idx) {
            if (app.GetViews().size() > 30) {
              return false;
            }

            // check if the app is in the correct dataset
            switch (type) {
              case DatasetType::ALL:
                return true;
              case DatasetType::TRAIN:
                return false;
              case DatasetType::VALID:
                return false;
              case DatasetType::TEST:
                return true;
            }
            CHECK(false) << "switch does not cover all the dataset types";
      }, [&cb, &type](App& app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id) {
            CHECK_EQ(ref_device.width, 1440);
            CHECK_EQ(ref_device.height, 2560);
            // take the smaller device
            std::vector<Device> gen_devices = {devices[0]};
            CHECK_EQ(gen_devices[0].width, 1400);
            CHECK_EQ(gen_devices[0].height, 2520);

            // ensure that the ground-truth is not used during test
            std::vector<App> empty_apps;
            for(auto& t_app: apps){
          	  empty_apps.push_back(EmptyApp(t_app));
            }
            return cb(app, (type == DatasetType::TEST) ? empty_apps : apps, ref_device, gen_devices, app_id);
      });
    }

  PropertyStats ForEachPlaystoreApp(
      DatasetType type,
      const std::function<SynResult(App, const std::vector<App>& apps, const Device&, const std::vector<Device>&, int app_id)>& cb) const {
	  return ForEachDPApp("data/neural_oracle/D_P/playstore_post.json", type, cb);
  }

   PropertyStats ForEachGithubDPApp(
      DatasetType type,
      const std::function<SynResult(App, const std::vector<App>& apps, const Device&, const std::vector<Device>&, int app_id)>& cb) const {
	  return ForEachDPApp("data/neural_oracle/D_P/github_post.json", type, cb);
  }

private:
  std::vector<int> CollectValidIds(const std::vector<Json::Value>& json_apps,
                                   const std::function<bool(const App&, int)>& contains_sample_cb,
                                   int num_samples = -1) const;
};


bool ComputeGeneralization(const App& ref_app, const App& syn_app,
                           const Device& ref_device, Device device,
                           Solver& solver, PropertyStats* stats);



class UserFeedbackSynthesis {
public:
  UserFeedbackSynthesis(const std::function<SynResult(App, const std::vector<App>& apps,
                                                      const Device&, const std::vector<Device>&,
                                                      int app_id)>&& cb) : fixed_views(0), total_views(0), base_synthesizer(cb)  {
    fallback_synthesizer = std::unique_ptr<GenSmtMultiDeviceProbOpt>(new GenSmtMultiDeviceProbOpt(true));
  }

  SynResult Synthesize(App app, const std::vector<App>& apps,
                       const Device& ref_device, const std::vector<Device>& devices,
                       int app_id);

  std::atomic<int> fixed_views;
  std::atomic<int> total_views;

private:

  bool AddInconsistentViewToSpec(const std::vector<App>& apps,
                                 const Device& ref_device, const std::vector<Device>& devices,
                                 const SynResult& res,
                                 std::vector<std::unordered_set<int>>& per_device_fixed_views,
                                 std::vector<App>& gen_apps) const;

  int NumInconsistentViewsNotInSpec(const std::vector<App>& apps,
                                    const Device& ref_device, const std::vector<Device>& devices,
                                    const SynResult& res,
                                    std::vector<std::unordered_set<int>>& per_device_fixed_views) const;

  const std::function<SynResult(App, const std::vector<App>& apps, const Device&, const std::vector<Device>&, int app_id)> base_synthesizer;
  thread_local static Solver solver;
  std::unique_ptr<GenSmtMultiDeviceProbOpt> fallback_synthesizer;
};

#endif //CC_SYNTHESIS_DATASET_UTIL_H
