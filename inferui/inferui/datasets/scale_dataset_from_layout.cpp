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

DEFINE_string(out_file, "data/neural_oracle/D_P/data.json", "Generate file containing the processed input specification");


Device GetResizedDevice(const Device& device) {
  return Device(device.width * 2, device.height * 2);
}


bool GenerateResizedOutputs(const App& app,
                            const Device& ref_device, const std::vector<Device>& out_devices,
                            Solver& solver, Json::Value* data) {
  for (const auto &device : out_devices) {
    App resized_app = app;
    TryResizeView(resized_app, resized_app.GetViews()[0], ref_device, device);
    resized_app = JsonToApp(solver.sendPost(ScaleApp(resized_app.ToJSON(), 2)));

    if (!ViewsInsideScreen(resized_app)) {
      return false;
    }

    JsonAppSerializer::AddScreenToJson(
        resized_app,
        GetResizedDevice(device), data);
  }
  return true;
}


int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  auto synthesizer = GenSmtMultiDeviceProbOpt(true);
  Device ref_device(720, 1280);
  std::vector<Device> devices({Device(682, 1032), Device(768, 1380)});

  PropertyStats stats;
  Solver solver;

  std::vector<Device> out_devices = {Device(720, 1280), Device(700, 1260), Device(740, 1300)};
  std::atomic<int> total(0), success(0), not_matching(0), unsat(0);

  std::vector<ProtoApp> screens;
  ForEachValidApp(FLAGS_test_data, [&](const ProtoApp& app) {
    screens.push_back(app);
  });

  std::vector<Json::Value> results(screens.size());
  std::mutex mutex;
#pragma omp parallel for private(solver) schedule(guided)
  for (size_t app_id = 0; app_id < screens.size(); app_id++) {
    const ProtoScreen &screen = screens[app_id].screens(0);

    Timer timer;
    timer.Start();

    App app(screen, true);
    if (app.GetViews().size() <= 2) continue;
    if (app.GetViews().size() > 30) continue;

    total++;
    app.InitializeAttributes(screen);
    if (!CanResizeView(app)) {
      continue;
    }

    Json::Value layout = solver.sendPost(app.ToJSON());
    App rendered_app = JsonToApp(layout);
    if (!AppMatch(app, rendered_app)) {
      not_matching++;
      continue;
    }

    // Use the synthesized layout to compute resolutions on different devices and serialize them to JSON
    Json::Value data(Json::objectValue);
    data["filename"] = screens[app_id].file_name();
    data["packagename"] = screens[app_id].package_name();
    if (!GenerateResizedOutputs(app, ref_device, out_devices, solver, &data)) {
      LOG(INFO) << "Skipping App that generated views outside screen!";
      LOG(INFO) << "Success: " << success << " / " << total;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
      continue;
    }
    results[app_id] = data;

    // (Optional) Synthesize layout and compute its generalization
    auto res = synthesizer.Synthesize(App(app), ref_device, devices);
    if (res.status != Status::SUCCESS) {
      LOG(INFO) << "Success: " << success << " / " << total;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
      unsat++;
      continue;
    }

    PrintApp(res.app);
    if (!TryFixInconsistencies(&res.app, solver)) {
      LOG(INFO) << "Synthesized Layout is does not match layout renderer";
      LOG(INFO) << "Success: " << success << " / " << total;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
      continue;
    }

    NormalizeMargins(&res.app, solver);

    {
      std::lock_guard<std::mutex> lock(mutex);

      for (const auto& device : devices) {
        App resized_app = app;
        TryResizeView(resized_app, resized_app.GetViews()[0], ref_device, device);
        resized_app = JsonToApp(solver.sendPost(resized_app.ToJSON()));

        ComputeGeneralization(resized_app, res.app, ref_device, device, solver, &stats);
      }

      success++;
      LOG(INFO) << "Success: " << success << " / " << total;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
    }


  }

  Json::FastWriter writer;
  std::ofstream out_file;
  out_file.open(FLAGS_out_file);
  for (auto& value : results) {
    if (value.empty()) continue;
    out_file << writer.write(value);
  }
  out_file.close();

  LOG(INFO) << "Success: " << success << " / " << total;
  LOG(INFO) << "Unsat: " << unsat << " / " << total;
  LOG(INFO) << "Not Matching: " << not_matching << " / " << total;
  stats.Dump();

  return 0;
}
