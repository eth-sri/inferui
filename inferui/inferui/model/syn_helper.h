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

#ifndef CC_SYNTHESIS_SYN_HELPER_H
#define CC_SYNTHESIS_SYN_HELPER_H

#include "inferui/model/model.h"
#include "inferui/layout_solver/solver.h"

// Try to fix inconsistencies of z3 solver and the Android layout solver by adjusting margins
// @returns true if rendering synthesized layout (in ref_app) using solver leads to same positions
bool TryFixInconsistencies(App* ref_app, Solver& solver);

// Try to replace margings (1,0) with (0,0)
void NormalizeMargins(App* ref_app, Solver& solver);

Json::Value DeviceToJson(const Device& device);

class JsonAppSerializer {
public:

  static std::vector<Json::Value> readDirectory(const std::string& path);
  static std::vector<Json::Value> readFile(const std::string& path);
  static int JsonToApps(const Json::Value& request,
      App& app, std::vector<App>& apps,
      Device& ref_device, std::vector<Device>& devices);

  static Json::Value ToJson(const App& app, const std::vector<App>& apps,
                            const Device& ref_device, const std::vector<Device>& devices);

  static Json::Value ScreenToJson(const App& app, const Device& device);
  static void AddScreenToJson(const App& app, const Device& device, Json::Value* data);
};

App EmptyApp(const App& ref, const Device& device);
App EmptyApp(const App& ref);

App KeepFirstNViews(const App& ref, int num_views);

bool ViewMatch(const View& A, const View& B);
bool AppMatch(const App& ref_app, const App& syn_app);

View JsonToView(const Json::Value& value);

App JsonToApp(const Json::Value& layout);

Json::Value ScaleApp(Json::Value value, double factor);

void ScaleAttributes(App& app, double scaling_factor);

double askOracle(
		const App& candidate, //the candidate for which the probability should be predicted
		Solver& solver,
		const Device device, //the device on which the candidate coordinates are valid on
		const std::string oracleType, //the type of oracle to be selected for the evaluation
		const std::string dataset, //name of the dataset on which we are evaluation/generating data on
		const std::string filename, //name of the file for which we generated a candidate
		const App& targetApp, //complete app on the specified device (with all views)
		const App& originalApp //app with coordinates on the original device
		);

void writeAppData(std::string targetPath, std::string name, const std::vector<std::vector<App>>& all_candidateDeviceApps, const std::vector<App>& targetApps, const App& originalApp);

#endif //CC_SYNTHESIS_SYN_HELPER_H
