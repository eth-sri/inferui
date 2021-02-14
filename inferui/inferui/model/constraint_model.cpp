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

#include "constraint_model.h"
#include "util/recordio/recordio.h"
#include "base/fileutil.h"
#include "inferui/layout_solver/solver.h"
#include "syn_helper.h"

DEFINE_double(scaling_factor, 1.0, "Scaling factor with which to resize applications.");


int NumIntersections(const View& src, const View& tgt, const std::vector<View>& views) {
  int num_intersections = 0;

  std::pair<int, int> xpoints = ClosestPointIntersection(src.xleft, src.xright, tgt.xleft, tgt.xright);
  std::pair<int, int> ypoints = ClosestPointIntersection(src.ytop, src.ybottom, tgt.ytop, tgt.ybottom);
  LineSegment segment(xpoints.first, ypoints.first, xpoints.second, ypoints.second);

  for (const View& view : views) {
    if (view == src || view == tgt) {
      continue;
    }

    if (segment.IntersectsLoose(view)) {
      num_intersections++;
    }
  }

  return num_intersections;
}

namespace Features {
  float GetDistance(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    const LineSegment &segment = LineTo(&src, &tgt, type);
    return std::round(segment.Length());
  }

  float NumIntersections(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    int num_intersections = 0;

    const LineSegment &segment = LineTo(&src, &tgt, type);
    for (const View& view : views) {
      if (view == src || view == tgt) {
        continue;
      }

      if (segment.Intersects(view)) {
        num_intersections++;
      }
    }

    return num_intersections;
  }

  float GetType(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    return static_cast<int>(type);
  }

  float GetAngle(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    const LineSegment &segment = LineTo(&src, &tgt, type);
    float angle = segment.GetAngle();
    if (std::isnan(angle)) {
      angle = -9999;
    } else {
      angle = std::round(angle);
      if (angle == -180) {
        angle = 180;
      }
    }
    return angle;
  }

  float GetMargin(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    return value;
  }

  float GetViewSize(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    return static_cast<int>(size);
  }

  float GetViewName(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    return FingerprintCat(static_cast<int>(size), FingerprintMem(src.name.c_str(), src.name.size()));
  }

  float GetViewDimensionRatio(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views) {
    if (ConstraintTypeToOrientation(type) == Orientation::HORIZONTAL) {
      return static_cast<int>(size) + 10 * (1 + (src.width() / 16));
    } else {
      return static_cast<int>(size) + 10 * (1 + (src.height() / 16));
    }
  }
}


namespace ConstraintModel {

  std::unique_ptr<AttrConstraintModel> GetMarginModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "MarginModel",
        Feature("Margin", Features::GetMargin),
        {
            "Horizontal Relative Margin",
            "Vertical Relative Margin",
            "Horizontal Centering Margin",
            "Vertical Centering Margin",
        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            1, //T2T,
            1, //T2B,
            1, //B2T,
            1, //B2B,

            2, //L2LxR2L,
            2, //L2LxR2R,
            2, //L2RxR2L,
            2, //L2RxR2R,

            3, //T2TxB2T,
            3, //T2TxB2B,
            3, //T2BxB2T,
            3, //T2BxB2B,
        }
    ));
  }

  std::unique_ptr<AttrConstraintModel> GetOrientationModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "OrientationModel",
        Feature("Orientation", Features::GetAngle),
        {
            ConstraintTypeStr(ConstraintType::L2L),
            ConstraintTypeStr(ConstraintType::L2R),
            ConstraintTypeStr(ConstraintType::R2L),
            ConstraintTypeStr(ConstraintType::R2R),

            ConstraintTypeStr(ConstraintType::T2T),
            ConstraintTypeStr(ConstraintType::T2B),
            ConstraintTypeStr(ConstraintType::B2T),
            ConstraintTypeStr(ConstraintType::B2B),

            "Horizontal Centering Orientation",
            "Vertical Centering Orientation",
//          ConstraintTypeStr(ConstraintType::L2LxR2L),
//          ConstraintTypeStr(ConstraintType::L2LxR2R),
//          ConstraintTypeStr(ConstraintType::L2RxR2L),
//          ConstraintTypeStr(ConstraintType::L2RxR2R),
//
//          ConstraintTypeStr(ConstraintType::T2TxB2T),
//          ConstraintTypeStr(ConstraintType::T2TxB2B),
//          ConstraintTypeStr(ConstraintType::T2BxB2T),
//          ConstraintTypeStr(ConstraintType::T2BxB2B),

        }, {
            0, //L2L = 0,
            1, //L2R,
            2, //R2L,
            3, //R2R,

            4, //T2T,
            5, //T2B,
            6, //B2T,
            7, //B2B,

            8, //L2LxR2L,
            8, //L2LxR2R,
            8, //L2RxR2L,
            8, //L2RxR2R,

            9, //T2TxB2T,
            9, //T2TxB2B,
            9, //T2BxB2T,
            9, //T2BxB2B,
        }
    ));
  }

  std::unique_ptr<AttrConstraintModel> GetDistanceModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "DistanceModel",
        Feature("Distance", Features::GetDistance),
        {
            "Horizontal Relative Margin",
            "Vertical Relative Margin",
            "Horizontal Centering Margin",
            "Vertical Centering Margin",
        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            1, //T2T,
            1, //T2B,
            1, //B2T,
            1, //B2B,

            2, //L2LxR2L,
            2, //L2LxR2R,
            2, //L2RxR2L,
            2, //L2RxR2R,

            3, //T2TxB2T,
            3, //T2TxB2B,
            3, //T2BxB2T,
            3, //T2BxB2B,
        }
    ));
  }


  std::unique_ptr<AttrConstraintModel> GetTypeModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "TypeModel",
        Feature("Type", Features::GetType),
        {
            "All Types"

        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            0, //T2T,
            0, //T2B,
            0, //B2T,
            0, //B2B,

            0, //L2LxR2L,
            0, //L2LxR2R,
            0, //L2RxR2L,
            0, //L2RxR2R,

            0, //T2TxB2T,
            0, //T2TxB2B,
            0, //T2BxB2T,
            0, //T2BxB2B,
        }
    ));
  }


  std::unique_ptr<AttrConstraintModel> GetIntersectionModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "IntersectionModel",
        Feature("Intersection", Features::NumIntersections),
        {
            "All Types"
        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            0, //T2T,
            0, //T2B,
            0, //B2T,
            0, //B2B,

            0, //L2LxR2L,
            0, //L2LxR2R,
            0, //L2RxR2L,
            0, //L2RxR2R,

            0, //T2TxB2T,
            0, //T2TxB2B,
            0, //T2BxB2T,
            0, //T2BxB2B,
        }
    ));
  }

  std::unique_ptr<AttrConstraintModel> GetViewSizeModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "ViewSizeModel",
        Feature("ViewSize", Features::GetViewSize, true),
        {
            "Horizontal ViewSize",
            "Vertical ViewSize",
//            "Horizontal Centering ViewSize",
//            "Vertical Centering ViewSize",
        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            1, //T2T,
            1, //T2B,
            1, //B2T,
            1, //B2B,

            0, //L2LxR2L,
            0, //L2LxR2R,
            0, //L2RxR2L,
            0, //L2RxR2R,

            1, //T2TxB2T,
            1, //T2TxB2B,
            1, //T2BxB2T,
            1, //T2BxB2B,
        }
    ));
  }

  std::unique_ptr<AttrConstraintModel> GetViewSizeNameModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "ViewSizeNameModel",
        Feature("ViewNameSize", Features::GetViewName, true),
        {
            "Horizontal ViewSizeName",
            "Vertical ViewSizeName",
        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            1, //T2T,
            1, //T2B,
            1, //B2T,
            1, //B2B,

            0, //L2LxR2L,
            0, //L2LxR2R,
            0, //L2RxR2L,
            0, //L2RxR2R,

            1, //T2TxB2T,
            1, //T2TxB2B,
            1, //T2BxB2T,
            1, //T2BxB2B,
        }
    ));
  }

  std::unique_ptr<AttrConstraintModel> GetViewSizeDimensionRatioModel() {
    return std::unique_ptr<CountingFeatureModel>(new CountingFeatureModel(
        "ViewSizeDimensionRatioModel",
        Feature("ViewSizeDimensionRatioModel", Features::GetViewDimensionRatio, true),
        {
            "Horizontal ViewSizeDimensionRatioModel",
            "Vertical ViewSizeDimensionRatioModel",
        }, {
            0, //L2L = 0,
            0, //L2R,
            0, //R2L,
            0, //R2R,

            1, //T2T,
            1, //T2B,
            1, //B2T,
            1, //B2B,

            0, //L2LxR2L,
            0, //L2LxR2R,
            0, //L2RxR2L,
            0, //L2RxR2R,

            1, //T2TxB2T,
            1, //T2TxB2B,
            1, //T2BxB2T,
            1, //T2BxB2B,
        }
    ));
  }

}

ViewSizeModelWrapper::ViewSizeModelWrapper() : ProbModel("ViewSizeModel") {
//  AddModel(ConstraintModel::GetViewSizeModel(), 1.0);
//    AddModel(ConstraintModel::GetViewSizeNameModel(), 1.0);
  AddModel(ConstraintModel::GetViewSizeDimensionRatioModel(), 1.0);
}

void ViewSizeModelWrapper::Train(const std::string& data_path) {
  CHECK(FileExists(data_path.c_str())) << "Data file " << data_path << " does not exist!";
  CHECK_EQ(FLAGS_scaling_factor, 1) << "Scaling factor not implemented!";
  LOG(INFO) << "Training model...";
  int64_t start = GetCurrentTimeMicros();
  int app_id = 0, invalid_apps = 0;
  int num_constraints = 0;
  ForEachValidApp(data_path.c_str(), [&](const ProtoApp& app) {
    const ProtoScreen &screen = app.screens(0);

    App ref_app(screen, true);
    if (ref_app.GetViews().size() == 1) return;
    ref_app.InitializeAttributes(screen);

    for (const View& view : ref_app.GetViews()) {
      if (view.is_content_frame()) continue;

      AddAttr(view.attributes.at(Orientation::HORIZONTAL), ref_app.GetViews());
      AddAttr(view.attributes.at(Orientation::VERTICAL), ref_app.GetViews());
      num_constraints += 2;
    }

  });

  std::cerr << '\r';
  int64_t end = GetCurrentTimeMicros();
  LOG(INFO) << "Done in " << ((end - start) / 1000) << "ms";
  LOG(INFO) << "Num apps: " << app_id << ", Invalid apps: " << invalid_apps;
  LOG(INFO) << "Num constraints: " << num_constraints;
}

ConstraintModelWrapper::ConstraintModelWrapper() : ProbModel("FeatureModel") {
  AddModel(ConstraintModel::GetOrientationModel(), 2.0);
  AddModel(ConstraintModel::GetMarginModel(), 1.0);
  AddModel(ConstraintModel::GetTypeModel(), 1.0);
  AddModel(ConstraintModel::GetDistanceModel(), 1.0);
  AddModel(ConstraintModel::GetIntersectionModel(), 1.0);
//    AddModel(ConstraintModel::GetViewSizeModel(), 1.0);
  AddModel(ConstraintModel::GetViewSizeDimensionRatioModel(), 2.0);
  AddModel(std::unique_ptr<AttrConstraintModel>(new AttrConstraintSizeModel()), 1.0);
}


void ConstraintModelWrapper::Train(const std::string& data_path) {
  CHECK(FileExists(data_path.c_str())) << "Data file " << data_path << " does not exist!";
  LOG(INFO) << "Training model...";
  int64_t start = GetCurrentTimeMicros();

  std::set<std::string> blacklisted_apps = {
      "am.appwise.components.ni",
      "com.ajithvgiri.stopwatch",
      "com.csci150.newsapp.entirenews",
      "com.doctoror.fuckoffmusicplayer",
      "com.example.maple.weatherapp",
      "com.expoagro.expoagrobrasil",
      "com.framgia.fbook",
      "com.github.eyers",
      "com.levip.runtrack",
      "com.projects.mikhail.bitcoinprice",
      "com.zacharee1.systemuituner",
      "de.hsulm.blewirkungsgrad",
      "dev.mad.ussd4etecsa",
      "jp.ogiwara.test.lobitest",
      "me.barta.stayintouch",
      "org.videolan.vlc",
      "plantfueled.puppysitter",
      "stan.androiddemo",
      "win.reginer.reader",
      "za.co.dvt.android.showcase",
  };

  Solver solver;

  std::vector<ProtoScreen> screens;
  ForEachValidApp(data_path.c_str(), [&](const ProtoApp& app) {
    if (Contains(blacklisted_apps, app.package_name())) return;
    screens.push_back(app.screens(0));
  });

  LOG(INFO) << "Collecting Training Apps...";
  std::vector<App> apps(screens.size());
#pragma omp parallel for private(solver)
  for (size_t i = 0; i < screens.size(); i++) {
    const ProtoScreen& screen = screens[i];

    App ref_app(screen, true);
    if (ref_app.GetViews().size() == 1) continue;
    ref_app.InitializeAttributes(screen);

    Json::Value layout = solver.sendPost(ref_app.ToJSON());
    App rendered_app = JsonToApp(layout);
    if (!AppMatch(ref_app, rendered_app)) {
      continue;
    }

    if (FLAGS_scaling_factor != 1) {
      rendered_app = JsonToApp(solver.sendPost(ScaleApp(ref_app.ToJSON(), FLAGS_scaling_factor)));
    }
    rendered_app.seq_id_to_pos = ref_app.seq_id_to_pos;
    rendered_app.InitializeAttributes(screen);
    if (FLAGS_scaling_factor != 1) {
      ScaleAttributes(rendered_app, FLAGS_scaling_factor);
    }
    apps[i] = rendered_app;
  }

  LOG(INFO) << "Training...";
  int app_id = 0;
  int num_constraints = 0;
  for (const App& app : apps) {
    if (app.GetViews().size() == 0) continue;

    app_id++;
    for (const View& view : app.GetViews()) {
      if (view.is_content_frame()) continue;

      AddAttr(view.attributes.at(Orientation::HORIZONTAL), app.GetViews());
      AddAttr(view.attributes.at(Orientation::VERTICAL), app.GetViews());
      num_constraints += 2;
    }
  }

  std::cerr << '\r';
  int64_t end = GetCurrentTimeMicros();
  LOG(INFO) << "Done in " << ((end - start) / 1000) << "ms";
  LOG(INFO) << "Num apps: " << app_id;
  LOG(INFO) << "Num constraints: " << num_constraints;
}