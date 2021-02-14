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

#ifndef CC_SYNTHESIS_EVAL_UTIL_H
#define CC_SYNTHESIS_EVAL_UTIL_H


#include "inferui/model/model.h"
#include "inferui/synthesis/z3inference.h"
#include "inferui/model/constraint_model.h"
#include "inferui/model/synthesis.h"
#include "inferui/layout_solver/solver.h"

DECLARE_string(train_data);
DECLARE_string(test_data);

std::map<std::string, bool> CheckProperties(App ref_app, const Device& ref_device, const std::vector<Device>& devices);

void adjustViewsByUserConstraints(App* syn_app);
std::pair<bool, std::vector<App>> applyTransformations(Solver& solver, const App& app, const std::vector<App> apps, std::string model, const Device ref_device, const std::vector<Device>& devices, std::string dataset, int& lowerLimit, int& upperLimit, int& necessaryUserCorrectionsSmaller, int& necessaryUserCorrectionsBigger);

class Synthesizer {
public:

  Synthesizer(const std::string& name) : name(name) {}

  virtual SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views = true) const = 0;

  virtual SynResult Synthesize(App&& app) const = 0;

  virtual SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const = 0;

  virtual SynResult SynthesizeOracle(App&& app, const std::vector<Device>& devices, const std::string oracleType, const std::string dataset) const {
	  LOG(FATAL) << "Not Implemented";
  }

  //Synthesize a layout for multiple app specifications (different device sizes)
  virtual SynResult SynthesizeMultipleApps(App&& app, std::vector<App>& apps) const {
	  LOG(FATAL) << "Not Implemented";
  }

  SynResult Synthesize(App& app) {
    App tmp = app;
    return Synthesize(std::move(tmp));
  }

  SynResult SynthesizeOracle(App& app, const std::vector<Device>& devices, const std::string oracleType, const std::string dataset) {
	App tmp = app;
	return SynthesizeOracle(std::move(tmp), devices, oracleType, dataset);
  }

  SynResult SynthesizeUser(App& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) {
    App tmp = app;
    return SynthesizeUser(std::move(tmp), apps, cb);
  }

  SynResult SynthesizeMultipleApps(App& app, std::vector<App>& apps) {
    App tmp = app;
    return SynthesizeMultipleApps(std::move(tmp), apps);
  }

  const std::string name;
  ValueCounter<std::string> stats;
  std::vector<App> debugApps;
  std::string filename;
  Json::Value targetXML;
};

class GenUserConstraints : public Synthesizer {
public:
  GenUserConstraints() : Synthesizer("GenUserConstraints") {}

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    SynResult result(App(screen, only_constraint_views));
    result.app.InitializeAttributes(screen);
    result.status = Status::SUCCESS;
    return result;
  }

  SynResult Synthesize(App&& app) const {
    LOG(FATAL) << "Not Implemented";
//    SynResult result(std::move(app));
//    result.status = Status::SUCCESS;
//    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    LOG(FATAL) << "Not Implemented";
  }
};

class GenSmtBaseline : public Synthesizer {
public:
  GenSmtBaseline() : Synthesizer("GenSmtBaseline") {}

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    FullSynthesis syn;
    SynResult result(App(screen, only_constraint_views));
    result.status = syn.SynthesizeLayout(result.app);
    return result;
  }

  SynResult Synthesize(App&& app) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayout(result.app);
    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    LOG(FATAL) << "Not Implemented";
  }

private:

};

class GenSmtMultiDevice : public Synthesizer {
public:
  GenSmtMultiDevice(Device ref_device, std::vector<Device> devices) : Synthesizer("GenSmtMultiDevice"),
    ref_device(ref_device), devices(devices) {

  }

  GenSmtMultiDevice() : Synthesizer("GenSmtMultiDevice"),
                        ref_device(Device(720, 1280)), devices({Device(682, 1032), Device(768, 1280)}) {

  }

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    FullSynthesis syn;
    SynResult result(App(screen, only_constraint_views));
    result.status = syn.SynthesizeLayoutMultiDevice(result.app, ref_device, devices);
    return result;
  }

  SynResult Synthesize(App&& app) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDevice(result.app, ref_device, devices);
    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    LOG(FATAL) << "Not Implemented";
  }

private:
  Device ref_device;
  std::vector<Device> devices;
};

//class GenSmtMultiDeviceProb : public Synthesizer {
//public:
//  GenSmtMultiDeviceProb(Device ref_device, std::vector<Device> devices)
//      : Synthesizer("GenSmtMultiDeviceProb"),
//      ref_device(ref_device), devices(devices) {
//    models.Train(FLAGS_train_data);
//    models.Dump();
//  }
//
//  GenSmtMultiDeviceProb()
//      : Synthesizer("GenSmtMultiDeviceProb"),
//        ref_device(Device(720, 1280)), devices({Device(682, 1032), Device(768, 1280)}) {
//    LOG(INFO) << "Train data: " << FLAGS_train_data;
//    models.Train(FLAGS_train_data);
//    models.Dump();
//  }
//
//  void SetDevice(const Device& ref, const std::vector<Device>& all) {
//    ref_device = ref;
//    devices = all;
//  }
//
//  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
//    FullSynthesis syn;
//    SynResult result(App(screen, only_constraint_views));
//    result.status = syn.SynthesizeLayoutMultiDevice(result.app, &models, ref_device, devices);
//    return result;
//  }
//
//  SynResult Synthesize(App&& app) const {
//    FullSynthesis syn;
//    SynResult result(std::move(app));
//    result.status = syn.SynthesizeLayoutMultiDevice(result.app, &models, ref_device, devices);
//    return result;
//  }
//
//private:
//  ConstraintModelWrapper models;
//  Device ref_device;
//  std::vector<Device> devices;
//};

class GenSmtSingleDeviceProbOpt : public Synthesizer {
public:
  GenSmtSingleDeviceProbOpt(bool opt, Device ref_device)
      : Synthesizer(opt ? "GenSmtSingleDeviceProbOpt" : "GenSmtSingleDeviceProb"),
        opt(opt), ref_device(ref_device) {
    models.Train(FLAGS_train_data);
    models.Dump();
  }

  GenSmtSingleDeviceProbOpt(bool opt)
      : Synthesizer(opt ? "GenSmtSingleDeviceProbOpt" : "GenSmtSingleDeviceProb"),
        opt(opt), ref_device(Device(720, 1280)) {
    LOG(INFO) << "Train data: " << FLAGS_train_data;
    models.Train(FLAGS_train_data);
    models.Dump();
  }

  void SetDevice(const Device& ref) {
    ref_device = ref;
  }

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    FullSynthesis syn;
    SynResult result(App(screen, only_constraint_views));
    std::vector<Device> devices;
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref_device, devices, opt);
    return result;
  }

  SynResult Synthesize(App&& app) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    std::vector<Device> devices;
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref_device, devices, opt);
    return result;
  }

  SynResult SynthesizeOracle(App&& app, const std::vector<Device>& devices, const std::string oracleType, const std::string dataset) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutProbOracle(result.app, &models, ref_device, devices, opt, oracleType, dataset, debugApps, filename, result.syn_stats, targetXML);
    return result;
  }

  SynResult SynthesizeOracleTS(App& app, const std::vector<Device>& devices, const std::string oracleType, const std::string dataset, const Device refDevice, const std::vector<App>& refApps, const std::string& name, const Json::Value& xml) const {
    App tmp = app;
    FullSynthesis syn;
    SynResult result(std::move(tmp));
    result.status = syn.SynthesizeLayoutProbOracle(result.app, &models, refDevice, devices, opt, oracleType, dataset, refApps, name, result.syn_stats, xml);
    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDeviceProbUser(result.app, &models, ref_device, apps, opt, false, cb);
    return result;
  }

private:
  bool opt;
  ConstraintModelWrapper models;
  Device ref_device;
};

class GenSmtMultiDeviceProbOpt : public Synthesizer {
public:
  GenSmtMultiDeviceProbOpt(bool opt, Device ref_device, std::vector<Device> devices)
      : Synthesizer(opt ? "GenSmtMultiDeviceProbOpt" : "GenSmtMultiDeviceProb"),
        opt(opt), ref_device(ref_device), devices(devices) {
    models.Train(FLAGS_train_data);
    models.Dump();
  }

  GenSmtMultiDeviceProbOpt(bool opt)
      : Synthesizer(opt ? "GenSmtMultiDeviceProbOpt" : "GenSmtMultiDeviceProb"),
        opt(opt), ref_device(Device(720, 1280)), devices({Device(682, 1032), Device(768, 1280)}) {
//        opt(opt), ref_device(Device(720, 1280)), devices({Device(768, 1280)}) {
    LOG(INFO) << "Train data: " << FLAGS_train_data;
    //LOG(INFO) << "initopt" << opt;
    models.Train(FLAGS_train_data);
    models.Dump();
  }

  void SetDevice(const Device& ref, const std::vector<Device>& all) {
    ref_device = ref;
    devices = all;
  }

  void SetOpt(bool _opt){
	  opt = _opt;
  }

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    FullSynthesis syn;
    SynResult result(App(screen, only_constraint_views));
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref_device, devices, opt);
    return result;
  }

  SynResult Synthesize(App&& app) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref_device, devices, opt);
    return result;
  }

  SynResult Synthesize(App&& app, const Device& ref, const std::vector<Device>& all) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref, all, opt);
    return result;
  }

  SynResult Synthesize(App&& app, const Device& ref, std::vector<App>& apps) {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref, apps, opt);
    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDeviceProbUser(result.app, &models, ref_device, apps, opt, true, cb);
    return result;
  }

  SynResult SynthesizeMultipleApps(App&& app, std::vector<App>& apps) {
	FullSynthesis syn;
	SynResult result(std::move(app));

	result.status = syn.SynthesizeLayoutMultiAppsProb(result.app, &models, ref_device, apps, opt);
	/*if(opt){
		LOG(INFO) << "should be false";
	}
	*/
	LOG(INFO) << opt << "optstatus";
    return result;
  }

  SynResult SynthesizeMultipleAppsIterative(App&& app, std::vector<App>& apps,
                                            int max_candidates,
                                            const std::function<bool(int, const App&, const std::vector<App>&)>& candidate_cb,
                                            const std::function<std::vector<App>(int, const App&)>& predict_cb,
                                            const std::function<void(int)>& iter_cb) {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutIterative(result.app, &models, apps, opt, max_candidates, candidate_cb, predict_cb, iter_cb);
    return result;
  }

  SynResult SynthesizeMultipleApps(App&& app, std::vector<App>& apps, const Device& device) {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiAppsProb(result.app, &models, device, apps, opt);
    return result;
  }

  SynResult SynthesizeMultipleAppsSingleQuery(App&& app, std::vector<App>& apps) {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiAppsProbSingleQuery(result.app, &models, apps, opt);
    return result;
  }

  SynResult SynthesizeMultipleAppsSingleQueryCandidates(App&& app, std::vector<App>& apps,
                                                        const std::function<bool(const App&, const std::vector<App>&)>& cb) {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiAppsProbSingleQueryCandidates(result.app, &models, apps, opt, cb);
    return result;
  }

private:
  bool opt;
  ConstraintModelWrapper models;
  Device ref_device;
  std::vector<Device> devices;
};

class GenProbSynthesis : public Synthesizer {
public:
  GenProbSynthesis() : Synthesizer("GenProbSynthesis"), syn(&models) {
    models.Train(FLAGS_train_data);
    models.Dump();
  }

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    SynResult result(App(screen, only_constraint_views));
    syn.SynthesizeLayout(result.app);
    result.status = Status::SUCCESS;
    return result;
  }

  SynResult Synthesize(App&& app) const {
    SynResult result(std::move(app));
    syn.SynthesizeLayout(result.app);
    result.status = Status::SUCCESS;
    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    LOG(FATAL) << "Not Implemented";
  }

private:
  ConstraintModelWrapper models;
  LayoutSynthesis syn;
};

class RandomModel : public ProbModel {
public:
  RandomModel() : ProbModel("RandomModel") {
    std::srand(0);
  }

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const {
    return "RandomModel";
  };

  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    double val = -1 * ((double) rand() / (RAND_MAX));
    return val;
  };

};

// Synthesis using
class RandomProbSynthesis : public Synthesizer {
public:

  RandomProbSynthesis()
      : Synthesizer("RandomProbSynthesis"),
        ref_device(Device(720, 1280)) {
  }

  void SetDevice(const Device& ref) {
    ref_device = ref;
  }

  SynResult Synthesize(const ProtoScreen& screen, bool only_constraint_views) const {
    FullSynthesis syn;
    SynResult result(App(screen, only_constraint_views));
    std::vector<Device> devices;
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref_device, devices, true);
    return result;
  }

  SynResult Synthesize(App&& app) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    std::vector<Device> devices;
    result.status = syn.SynthesizeLayoutMultiDeviceProb(result.app, &models, ref_device, devices, true);
    return result;
  }

  SynResult SynthesizeUser(App&& app, std::vector<App>& apps, const std::function<bool(const App&)>& cb) const {
    FullSynthesis syn;
    SynResult result(std::move(app));
    result.status = syn.SynthesizeLayoutMultiDeviceProbUser(result.app, &models, ref_device, apps, true, false, cb);
    return result;
  }

private:
  RandomModel models;
  Device ref_device;
};




#endif //CC_SYNTHESIS_EVAL_UTIL_H
