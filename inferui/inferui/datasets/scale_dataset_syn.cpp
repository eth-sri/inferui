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

DEFINE_string(path, "data/rendered_rico/2plus_resolutions.json", "Path to directory with json files containing input specification");
DEFINE_string(out_file, "data/neural_oracle/D_S+/data.json", "Generate file containing the processed input specification");

bool ValidApps(const App& app, const std::vector<App>& apps) {
  for (const App& temp : apps) {
    if (temp.GetViews().size() != app.GetViews().size()) {
      return false;
    }
  }
  return true;
}

Device GetResizedDevice(const Device& device) {
  return Device(device.width * 4, device.height * 4);
}

bool GenerateResizedOutputs(const App& app,
                            const Device& ref_device, const std::vector<Device>& out_devices,
                            Solver& solver, Json::Value* data) {
  for (const auto &device : out_devices) {
    App resized_app = app;
    TryResizeView(resized_app, resized_app.GetViews()[0], ref_device, device);
    resized_app = JsonToApp(solver.sendPost(ScaleApp(resized_app.ToJSON(), 4)));

    Device target_device = GetResizedDevice(device);
    CHECK_EQ(resized_app.GetViews()[0].width(), target_device.width);
    CHECK_EQ(resized_app.GetViews()[0].height(), target_device.height);

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
  PropertyStats stats;
  Solver solver;

  std::vector<Device> out_devices = {Device(350, 630), Device(360, 640), Device(370, 650)};
  std::atomic<int> total(0), success(0);

  const auto json_apps = (DirectoryExists(FLAGS_path.c_str())) ?
                         JsonAppSerializer::readDirectory(FLAGS_path) :
                         JsonAppSerializer::readFile(FLAGS_path);

  std::vector<Json::Value> results(json_apps.size());
  std::mutex mutex;
#pragma omp parallel for private(solver) schedule(guided)
  for (size_t app_id = 0; app_id < json_apps.size(); app_id++) {
    const auto &root = json_apps[app_id];

    Timer timer;
    timer.Start();

    std::vector<App> apps;
    App app = App();
    Device ref_device = Device(0, 0);
    std::vector<Device> devices;
    JsonAppSerializer::JsonToApps(root, app, apps, ref_device, devices);

    if (!ValidApps(app, apps) || app.GetViews().size() > 80) {
      // For apps with a lot of views the probabilistic model does not scale
      LOG(INFO) << "App has a different number of views on different devices.";
      continue;
    }

    total++;
    // use all apps as specification
    auto res = synthesizer.SynthesizeMultipleApps(std::move(app), apps, ref_device);
    // baseline InferUI with only single app as specification
//    auto res = synthesizer.Synthesize(std::move(app), ref_device, devices);

    if (res.status != Status::SUCCESS) {
      LOG(INFO) << "Unsuccessful " << root["filename"].asString();
      LOG(INFO) << "Success: " << success << " / " << total;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
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

      for (size_t device_id = 0; device_id < devices.size(); device_id++) {
        const auto &device = devices[device_id];
        const App &resized_app = apps[device_id];
        if (!ComputeGeneralization(resized_app, res.app, ref_device, device, solver, &stats)) {
          LOG(INFO) << "Synthesized Layout does not match Reference Android Layout Renderer";
          LOG(INFO) << "Success: " << success << " / " << total;
          LOG(INFO) << "#Views: " << app.GetViews().size();
          LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
          continue;
        }
      }
    }

    // Use the synthesized layout to compute resolutions on different devices and serialize them to JSON
    Json::Value data(Json::objectValue);
    data["id"] = BaseName(root["id"].asString());
    if (!GenerateResizedOutputs(res.app, ref_device, out_devices, solver, &data)) {
      LOG(INFO) << "Skipping App that generated views outside screen!";
      LOG(INFO) << "Success: " << success << " / " << total;
      LOG(INFO) << "#Views: " << app.GetViews().size();
      LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
      continue;
    }
    {
      Json::Value resolutions(Json::arrayValue);
      resolutions.append(DeviceToJson(ref_device));
      for (const auto& device : devices) {
        resolutions.append(DeviceToJson(device));
      }
      data["reference_resolutions"] = resolutions;
    }
    results[app_id] = data;

    success++;
    LOG(INFO) << "Success: " << success << " / " << total;
    LOG(INFO) << "#Views: " << app.GetViews().size();
    LOG(INFO) << "Took " << std::round(timer.GetMilliSeconds()/1000) << "s";
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
  stats.Dump();

  return 0;
}
