#include <glog/logging.h>
#include "inferui/layout_solver/solver.h"
#include "inferui/model/model.h"
#include "inferui/eval/eval_util.h"
#include "inferui/eval/eval_app_util.h"

int main(int argc, char** argv) {
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  Solver solver;

  GenSmtMultiDeviceProbOpt syn(true);
  App syn_app;
  syn_app.AddView(View(0, 0, 720, 1280, "ConstraintLayout", 0, "parent"));
  syn_app.AddView(View(321, 601, 400, 680, "Button", 1, "@+id/view1"));

  syn_app.GetViews().back().attributes.insert(std::make_pair(Orientation::HORIZONTAL,
      Attribute(ConstraintType::L2LxR2R, ViewSize::FIXED, 0, &syn_app.GetViews()[1], &syn_app.GetViews()[0], &syn_app.GetViews()[0])));

  syn_app.GetViews().back().attributes.insert(std::make_pair(Orientation::VERTICAL,
      Attribute(ConstraintType::T2TxB2B, ViewSize::FIXED, 0, &syn_app.GetViews()[1], &syn_app.GetViews()[0], &syn_app.GetViews()[0])));

  PrintApp(syn_app, false);

  Json::Value layout = solver.sendPost(syn_app.ToJSON());
  App rendered_app = JsonToApp(layout);
  PrintApp(rendered_app, false);

  syn_app.GetViews()[1].attributes.clear();
  syn_app.resizable.push_back(true);
  syn_app.resizable.push_back(true);
  SynResult res = syn.Synthesize(std::move(syn_app));
  PrintApp(res.app, true);

  return 0;
}
