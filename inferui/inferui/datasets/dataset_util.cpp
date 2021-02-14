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

#include "dataset_util.h"
#include "inferui/model/model.h"
#include "inferui/layout_solver/solver.h"
#include "inferui/model/syn_helper.h"
#include "inferui/eval/eval_app_util.h"
#include "inferui/eval/eval_util.h"

#include <glog/logging.h>

DEFINE_bool(fix_inconsistencies, true, "Iterate with Normalize and TryFixInconsistencies tricks.");
DEFINE_bool(base_syn_fallback, true, "Fallback to baseline synthesizer if the synthesis fails.");

void PropertyStats::Add(const Orientation& orientation, bool correct) {
  values[orientation].first++;
  if (correct) {
    values[orientation].second++;
  }
}

void PropertyStats::AddView(bool correct_horizontal, bool correct_vertical) {
  Add(Orientation::HORIZONTAL, correct_horizontal);
  Add(Orientation::VERTICAL, correct_vertical);
  if (correct_horizontal && correct_vertical) {
    fully_correct++;
  }
  total++;
}

void PropertyStats::Dump() const {
  LOG(INFO) << "Fully Correct: " << fully_correct << " / " << total << " (" << (fully_correct * 100.0 / total) << "%)";
  for (const Orientation& orientation : {Orientation::HORIZONTAL, Orientation::VERTICAL}) {
    LOG(INFO) << "\tOrientation " << orientation << ": " << values.at(orientation).second << " / " << values.at(orientation).first << " (" <<
              values.at(orientation).second * 100.0 / values.at(orientation).first << "%)";
  }

  LOG(INFO) << "Success: " << success_apps << " / " << total_apps
            << ", Inconsistent: " << inconsistent_apps
            << ", Failed: " << failed_syn_apps
			<< "(timeout: " << timeout_apps
			<< ", unsat: " << unsat_apps << ")";
  LOG(INFO) << "Fixed Views Stats: " << fixed_views << " / " << total_views
            << " (" << (fixed_views * 100.0 / total_views) << "%)";
}

bool ViewsInsideScreen(const App& app) {
  const View& root = app.GetViews()[0];
  for (const View& view : app.GetViews()) {
    if (view.xleft < root.xleft || view.xright > root.xright ||
        view.ytop < root.ytop || view.ybottom > root.ybottom) {
      return false;
    }
  }
  return true;
}


App LayoutResizeApp(App resized_syn_app, const Device& ref_device, const Device& device, Solver& solver) {
  TryResizeView(resized_syn_app, resized_syn_app.GetViews()[0], ref_device, device);
  return JsonToApp(solver.sendPost(resized_syn_app.ToJSON()));
}

bool ComputeGeneralization(const App& ref_app, const App& syn_app,
                           const Device& ref_device, Device device,
                           Solver& solver, PropertyStats* stats) {
  App resized_syn_app = LayoutResizeApp(syn_app, ref_device, device, solver);

  bool correct = true;
  for (size_t j = 1; j < ref_app.GetViews().size(); j++) {
    const View& src_view = ref_app.GetViews()[j];
    const View& syn_view = resized_syn_app.GetViews()[j];

    stats->AddView(
        src_view.xleft == syn_view.xleft && src_view.xright == syn_view.xright,
        src_view.ytop == syn_view.ytop && src_view.ybottom == syn_view.ybottom
    );
    correct = correct &&
              src_view.xleft == syn_view.xleft && src_view.xright == syn_view.xright &&
              src_view.ytop == syn_view.ytop && src_view.ybottom == syn_view.ybottom;
  }
  return correct;
}

/*
 * Dataset Iterators
 */

PropertyStats DatasetIterators::ForEachApp(
    const std::string& path,
    const std::function<bool(const App&, int)>& contains_sample_cb,
    const std::function<SynResult(App&, const std::vector<App>&, const Device&, const std::vector<Device>&, int appId)>& cb,
    int num_samples) const {

  PropertyStats stats;
  Solver solver;

  std::unique_ptr<GenSmtMultiDeviceProbOpt> synthesizer;
  if (FLAGS_base_syn_fallback) {
    // synthesizer instantiation is expensive so do it only if its needed
    synthesizer = std::unique_ptr<GenSmtMultiDeviceProbOpt>(new GenSmtMultiDeviceProbOpt(true));
  }

  std::atomic<int> total_apps(0), success_apps(0), inconsistent_apps(0), unsat_apps(0), timeout_apps(0), failed_syn_apps(0);

  const auto json_apps = JsonAppSerializer::readFile(path);
  // Collect apps which should be evaluated first to ensure that the parallelization is efficient
  std::vector<int> valid_ids = CollectValidIds(json_apps, contains_sample_cb, num_samples);

  std::mutex mutex;
#pragma omp parallel for private(solver) schedule(guided) //reduction(plusstats : correct_view_rank)
  for (size_t i = 0; i < valid_ids.size(); i++) {
    const auto &root = json_apps[valid_ids[i]];

    Timer timer;
    timer.Start();

    std::vector<App> apps;
    App app = App();
    Device ref_device = Device(0, 0);
    std::vector<Device> devices;
    int app_idx = JsonAppSerializer::JsonToApps(root, app, apps, ref_device, devices);

    CHECK(contains_sample_cb(app, app_idx)) << "CollectValidIds is expect to return only valid apps!";

    LOG(INFO) << "Synthesizing Layout for app: " << i;
    PrintApp(app, false);

    total_apps++;
    auto res = cb(app, apps, ref_device, devices, app_idx);

    // Stats
    if (res.status != Status::SUCCESS) {
      LOG(INFO) << "Unsuccessful " << root["filename"].asString();
      LOG(INFO) << "Success: " << success_apps << " / " << total_apps;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
      failed_syn_apps++;
      if (res.status == Status::TIMEOUT) {
    	  timeout_apps++;
      } else if (res.status == Status::UNSAT) {
        unsat_apps++;
      }
      if (FLAGS_base_syn_fallback) {
        // to make the numbers in evaluation comparable among different models, they should all succeed
        // on the same subset of apps. When a model fails, instead of removing this app from the evaluation
        // or assigning zero score, we fallback to a baseline synthesis

        CHECK(synthesizer);
        std::vector<App> input_apps;
        res = synthesizer->SynthesizeMultipleAppsSingleQuery(std::move(app), input_apps);
        if (res.status != Status::SUCCESS) {
          continue;
        }
      } else {
        continue;
      }
    }

    if (FLAGS_fix_inconsistencies) {
      if (!TryFixInconsistencies(&res.app, solver)) {
        LOG(INFO) << "Synthesized Layout is does not match layout renderer";
        LOG(INFO) << "Success: " << success_apps << " / " << total_apps;
        LOG(INFO) << "#Views: " << res.app.GetViews().size();
        LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
        inconsistent_apps++;
      }

      NormalizeMargins(&res.app, solver);
    }

    LOG(INFO) << "Synthesized App:";
    LOG(INFO) << res.app.ToJSON();

    {
      std::lock_guard<std::mutex> lock(mutex);

      for (size_t device_id = 0; device_id < devices.size(); device_id++) {
        const auto &device = devices[device_id];
        const App &resized_app = apps[device_id];
        if (!ComputeGeneralization(resized_app, res.app, ref_device, device, solver, &stats)) {
          LOG(INFO) << "Synthesized Layout does not match Reference Android Layout Renderer";
          LOG(INFO) << "Success: " << success_apps << " / " << total_apps;
          LOG(INFO) << "#Views: " << res.app.GetViews().size();
          LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
        }
      }

    }

    success_apps++;
    LOG(INFO) << "Success: " << success_apps << " / " << total_apps;
    LOG(INFO) << "#Views: " << res.app.GetViews().size();
    LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
  }

  LOG(INFO) << "Success: " << success_apps << " / " << total_apps;
  LOG(INFO) << "Inconsistent:" << inconsistent_apps;
  LOG(INFO) << "Failed Synthesis:" << failed_syn_apps;

  stats.total_apps = total_apps;
  stats.success_apps = success_apps;
  stats.inconsistent_apps = inconsistent_apps;
  stats.failed_syn_apps = failed_syn_apps;
  stats.unsat_apps = unsat_apps;
  stats.timeout_apps = timeout_apps;
  return stats;
}

std::vector<int> DatasetIterators::CollectValidIds(const std::vector<Json::Value>& json_apps,
                                 const std::function<bool(const App&, int)>& contains_sample_cb,
                                 int num_samples) const {
  std::vector<int> valid_ids;
  for (size_t app_id = 0; app_id < json_apps.size(); app_id++) {
    if (num_samples != -1 && static_cast<int>(valid_ids.size()) >= num_samples) {
      break;
    }
    const auto &root = json_apps[app_id];

    std::vector<App> apps;
    App app = App();
    Device ref_device = Device(0, 0);
    std::vector<Device> devices;
    int app_idx = JsonAppSerializer::JsonToApps(root, app, apps, ref_device, devices);

    if (!contains_sample_cb(app, app_idx)) {
      continue;
    }
    valid_ids.push_back(app_id);
  }
  return valid_ids;
}

thread_local Solver UserFeedbackSynthesis::solver;

std::vector<int> FindNonEqualViews(const App& ground_truth, const App& app) {
  std::vector<int> res;
  for (size_t j = 1; j < ground_truth.GetViews().size(); j++) {
    const View &src_view = ground_truth.GetViews()[j];
    const View &syn_view = app.GetViews()[j];

    bool correct = src_view.xleft == syn_view.xleft && src_view.xright == syn_view.xright &&
                   src_view.ytop == syn_view.ytop && src_view.ybottom == syn_view.ybottom;
    if (!correct) {
      res.push_back(j);
    }
  }
  return res;
}

bool UserFeedbackSynthesis::AddInconsistentViewToSpec(const std::vector<App>& apps,
                                                      const Device& ref_device, const std::vector<Device>& devices,
                                                      const SynResult& res,
                                                      std::vector<std::unordered_set<int>>& per_device_fixed_views,
                                                      std::vector<App>& gen_apps) const {
  for (size_t device_id = 0; device_id < devices.size(); device_id++) {
    const App &ground_truth = apps[device_id];
    App resized_syn_app = LayoutResizeApp(res.app, ref_device, devices[device_id], solver);
    std::vector<int> view_ids = FindNonEqualViews(ground_truth, resized_syn_app);

    if (view_ids.empty()) {
      continue;
    }

    // fix view that has not yet been fixed,
    // it's possible that even it the view is fixed it's position is wrong due to the synthesizer inconsistency
    for (int view_id : view_ids) {
      if (Contains(per_device_fixed_views[device_id], view_id)) {
        continue;
      }

      // fix the view position (synthetic user feedback)
      gen_apps[device_id].GetViews()[view_id].setPosition(ground_truth.GetViews()[view_id]);
      per_device_fixed_views[device_id].insert(view_id);
      return true;
    }
  }
  return false;
}

int UserFeedbackSynthesis::NumInconsistentViewsNotInSpec(const std::vector<App>& apps,
                                                          const Device& ref_device, const std::vector<Device>& devices,
                                                          const SynResult& res,
                                                          std::vector<std::unordered_set<int>>& per_device_fixed_views) const {
  int num_inconsistent_views = 0;
  for (size_t device_id = 0; device_id < devices.size(); device_id++) {
    const App &ground_truth = apps[device_id];
    App resized_syn_app = LayoutResizeApp(res.app, ref_device, devices[device_id], solver);
    std::vector<int> view_ids = FindNonEqualViews(ground_truth, resized_syn_app);

    for (int view_id : view_ids) {
      if (!Contains(per_device_fixed_views[device_id], view_id)) {
        num_inconsistent_views++;
      }
    }
  }
  return num_inconsistent_views;
}

SynResult UserFeedbackSynthesis::Synthesize(App app, const std::vector<App>& apps,
                     const Device& ref_device, const std::vector<Device>& devices,
                     int app_id) {
  std::vector<App> gen_apps;
  for (const auto& _app : apps) {
    gen_apps.emplace_back(EmptyApp(_app));

    // sanity check
    for (const auto& view : _app.GetViews()) {
      CHECK(view.HasFixedPosition()) << "Expects view to have concrete positions for UserFeedback Evaluation";
    }
  }

  std::vector<std::unordered_set<int>> per_device_fixed_views(apps.size());
  bool view_added;
  SynResult res, last_success;
  do {
    res = base_synthesizer(app, gen_apps, ref_device, devices, app_id);
    if (res.status != Status::SUCCESS) {
      if (FLAGS_base_syn_fallback) {
        CHECK(fallback_synthesizer);
        res = fallback_synthesizer->SynthesizeMultipleAppsSingleQuery(std::move(App(app)), gen_apps);
      }
      if (res.status != Status::SUCCESS) {
        break;
      }
    }
    if (res.status == Status::SUCCESS) {
      last_success = res;
    }

    if (FLAGS_fix_inconsistencies) {
      TryFixInconsistencies(&res.app, solver);
      NormalizeMargins(&res.app, solver);
    }

    view_added = AddInconsistentViewToSpec(apps, ref_device, devices, res, per_device_fixed_views, gen_apps);
  } while (view_added);

  if (last_success.status != Status::SUCCESS) {
    CHECK(fallback_synthesizer);
    last_success = fallback_synthesizer->SynthesizeMultipleAppsSingleQuery(std::move(App(app)), gen_apps);
  }

  CHECK_EQ(last_success.status, Status::SUCCESS);
  if (res.status != Status::SUCCESS) {
    res = last_success;
  }

  int num_inconsistent_views = NumInconsistentViewsNotInSpec(apps, ref_device, devices, res, per_device_fixed_views);
  if (res.status != Status::SUCCESS) {
    CHECK_EQ(num_inconsistent_views, 0);
  }
  fixed_views.fetch_add(num_inconsistent_views);

  total_views.fetch_add(static_cast<int>((app.GetViews().size() - 1) * apps.size()));
  for (const auto& views : per_device_fixed_views) {
    fixed_views.fetch_add(views.size());
  }
  return res;
}
