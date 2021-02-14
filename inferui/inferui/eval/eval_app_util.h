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

#ifndef CC_SYNTHESIS_INFERUI_EVAL_EVAL_APP_UTIL_H_
#define CC_SYNTHESIS_INFERUI_EVAL_EVAL_APP_UTIL_H_

#include "inferui/model/uidump.pb.h"
#include "inferui/model/model.h"
#include "inferui/model/constraint_model.h"
#include "inferui/model/synthesis.h"
#include "inferui/model/syn_helper.h"

DECLARE_bool(user_corrects);
DECLARE_string(experiment_type);
DECLARE_bool(correct_cand_exp);
DECLARE_bool(generate_data);

enum Mode
{
    baseline, //default, what has been there before
	baselineFallback,
    multiAppsVerification, //render with all 3 apps at once
    oracleMLP,
    oracleCnnImageOnly,
	oracleCnnMLP,
	ensembleRnnCnnAbs,
	ensembleRnnCnnBoth,
	doubleRNN
};

std::string ModeTypeStr(const Mode& mode);
std::string ModeTypeOracleStr(const Mode& mode);

bool CheckViewName(const Json::Value& rview, const View& sview);
bool CheckPosition(int expected, int actual, int root);
bool CheckViewLocations(const Json::Value& rview, const View& sview, const View& root);

void AnalyseAppMatch(const App& ref_app, const App& syn_app);
void AnalyseAppMatchLayouts(const App& ref_app, const App& syn_app);
bool AppMatchApprox(const App& ref_app, const App& syn_app);
bool SingleViewMatchApprox(const View& ref_view, const View& syn_view);
bool SingleViewMatch (const View& ref_view, const View& syn_view);
std::pair<int,int> ViewMatchApprox(const App& ref_app, const App& syn_app);
std::pair<int,int> ViewMatchApproxForOrientation(const App& ref_app, const App& syn_app);
std::pair<double, int> IntersectionOfUnion(const App& ref_app, const App& syn_app);

std::pair<int,int> ViewMatch(const App& ref_app, const App& syn_app);

void JsonToApps(const Json::Value& request, App& app, std::vector<App>& apps, Device& ref_device, std::vector<Device>& devices);

bool CanResizeView(const App& ref_app);
void TryResizeView(const App& ref_app, View& new_root, const Device& ref_device, const Device& device);
std::pair<int, int> layoutMatch(Json::Value& ref_app, Json::Value& syn_app, bool vertical);

Json::Value AppConstraintsToJSON(const App& app, const std::vector<int> swaps  = std::vector<int>());
#endif /* CC_SYNTHESIS_INFERUI_EVAL_EVAL_APP_UTIL_H_ */
