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

DEFINE_string(path, "data/rendered_rico/2plus_resolutions.json", "Path to directory with json files containing input specification");
DEFINE_int32(max_candidates, 16, "Maximum Number of Candidate Layouts at each iteration");


SynResult SynthesizeIteratively(
    const std::vector<App>& apps, App& app,
    const std::vector<Device>& devices, const Device& ref_device,
    GenSmtMultiDeviceProbOpt& synthesizer, std::vector<std::atomic<int>>& correct_view_rank) {

  std::vector<App> refinement_apps;
  // pick smaller device for refinement
  refinement_apps.push_back(EmptyApp(apps[0]));
  std::vector<Device> ref_devices = {devices[0]};

  std::vector<App> best_candidate_apps;
  bool found_correct_candidate = false;
  auto res = synthesizer.SynthesizeMultipleAppsIterative(std::move(app), refinement_apps, FLAGS_max_candidates,
     [&](int candidate_id, const App& candidate_app, const std::vector<App>& candidate_device_apps){

       /* last view in candidate_device_apps is the candidate view that we are trying to predict at this iteration
        * Each time this callback is invoked, the last view has a new position, different to all the previous ones
        *
        * return true: continues generating candidates until max_candidates or no more candidates exists
        * return false: selects current candidate as the correct one and proceeds to generating candidates for next view
        */
       int view_id = candidate_app.GetViews().size() - 1;

       if (ViewMatch(candidate_device_apps[0].GetViews()[view_id], apps[0].GetViews()[view_id])) {
         correct_view_rank[candidate_id]++;
         found_correct_candidate = true;
         // positive sample, not guaranteed to be generated as candidate

         best_candidate_apps = candidate_device_apps;
       }

       return true;
     }, [&](int num_views, const App& candidate_app){
        /*
         * Called if the previous callback never returned false.
         * In evaluation, we would pick here the argmax of all the candidates returned in the previous callback (for this iteration)
         * In dataset generation we return the ground-truth
         */

        std::vector<App> res = { KeepFirstNViews(apps[0], num_views) };
        return res;
     }, [&found_correct_candidate, &correct_view_rank](int num_views) {
        /*
         * Called after a view position have been fixed (regardless of whether using first or second callback)
         */
        if (!found_correct_candidate) {
          correct_view_rank[FLAGS_max_candidates]++;
        }

        found_correct_candidate = false;
     });
  return res;
}

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  auto synthesizer = GenSmtMultiDeviceProbOpt(true);
  auto dataset_iterator = DatasetIterators();

  std::vector<std::atomic<int>> correct_view_rank(FLAGS_max_candidates + 1);
  for (auto& value : correct_view_rank) {
    value.store(0);
  }

  PropertyStats stats = dataset_iterator.ForEachApp(FLAGS_path,
      [&](App& app, const std::vector<App>& apps, const Device& ref_device, const std::vector<Device>& devices, int app_id){
    // Everything here needs to be thread safe
        return SynthesizeIteratively(apps, app, devices, ref_device, synthesizer, correct_view_rank);
  }, 20);

  stats.Dump();

  LOG(INFO) << "Correct View Rank";
  for (int i = 1; i < FLAGS_max_candidates; i++) {
    LOG(INFO) << StringPrintf("\t%2d: %d", i, correct_view_rank[i].load());
  }
  LOG(INFO) << StringPrintf("\t%2d+: %d", FLAGS_max_candidates, correct_view_rank[FLAGS_max_candidates].load());
  return 0;
}
