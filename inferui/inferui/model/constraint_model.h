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

#ifndef CC_SYNTHESIS_CONSTRAINT_MODEL_H
#define CC_SYNTHESIS_CONSTRAINT_MODEL_H

#include <gflags/gflags_declare.h>

#include "model.h"
#include "base/fileutil.h"

DECLARE_double(scaling_factor);

class Feature {
public:
  typedef std::pair<float, float> Value;
  Feature(const std::string& name,
          const std::function<float(const View&, const View&, float, const ConstraintType&, const ViewSize&, const std::vector<View>&)>& fn,
          bool unary = false) : name(name), fn(fn), unary(unary) {

  }

  Value ValueAttr(const Attribute& attr, const std::vector<View>& views) const {
    if (IsRelationalAnchor(attr.type) || unary) {
      return std::make_pair(fn(*attr.src, *attr.tgt_primary, attr.value_primary, attr.type, attr.view_size, views), -1);
    } else {
      std::pair<ConstraintType, ConstraintType> anchors = SplitCenterAnchor(attr.type);
      return std::make_pair(fn(*attr.src, *attr.tgt_primary, attr.value_primary, anchors.first, attr.view_size, views), fn(*attr.src, *attr.tgt_secondary, attr.value_secondary, anchors.second, attr.view_size, views));
    }
  };

  const std::string name;
private:
  const std::function<float(const View&, const View&, float, const ConstraintType&, const ViewSize&, const std::vector<View>&)> fn;
  bool unary;
};

namespace std {
  template <> struct hash<std::pair<float, float>> {
    size_t operator()(const std::pair<float, float>& x) const {
      return x.first * 9781 + x.second;
    }
  };
}

inline std::ostream& operator<<(std::ostream& os, std::pair<float, float>& value) {
  os << "value(" << value.first << ", " << value.second << ")";
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::pair<float, float>& value) {
  os << "value(" << value.first << ", " << value.second << ")";
  return os;
}

int NumIntersections(const View& src, const View& tgt, const std::vector<View>& views);

namespace Features {
  float GetDistance(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);

  float NumIntersections(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);

  float GetType(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);

  float GetAngle(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);

  float GetMargin(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);

  float GetViewSize(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);
  float GetViewName(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);
  float GetViewDimensionRatio(const View& src, const View& tgt, float value, const ConstraintType &type, const ViewSize& size, const std::vector<View>& views);
}

class AttrConstraintModel {
public:
  AttrConstraintModel(const std::string& name) : name(name) {

  }

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const {
    std::stringstream ss;
    ss << name.c_str() << " " << AttrValue(attr, views) << ", " << AttrProb(attr, views);
    return ss.str();
  }

  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const = 0;

  virtual Feature::Value AttrValue(const Attribute& attr, const std::vector<View>& views) const = 0;

  virtual void AddAttr(const Attribute& attr, const std::vector<View>& views) = 0;
//  virtual void AddRelation(const Constants::Name& property, const View& src, const View& tgt, const std::vector<View>& views) = 0;

  virtual void SaveOrDie(FILE* file) const = 0;
  virtual void LoadOrDie(FILE* file) = 0;

  virtual void Dump(std::ostream& os) const = 0;

protected:
  const std::string name;
};


class AttrConstraintSizeModel : public AttrConstraintModel {
public:

  AttrConstraintSizeModel() : AttrConstraintModel("size") {
    probs = {
        0.3,
        0.1,
        0.03,
        0.029,
        0.028,
        0.025,
        0.01
    };
  }

  double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    size_t size = AttrValue(attr, views).first;
    if (size < probs.size()) {
      return probs[size];
    } else {
      return 0.002;
    }
  }

  Feature::Value AttrValue(const Attribute& attr, const std::vector<View>& views) const {
    return std::pair<float, float>(attr.size(), 0);
  }

  void AddAttr(const Attribute& attr, const std::vector<View>& views) {}
  void SaveOrDie(FILE* file) const {}
  void LoadOrDie(FILE* file) {}

  friend std::ostream& operator<<(std::ostream& os, const AttrConstraintSizeModel& model);

  void Dump(std::ostream& os) const {
    os << *this;
  }

private:
  std::vector<double> probs;
};

inline std::ostream& operator<<(std::ostream& os, const AttrConstraintSizeModel& model) {
  os << model.name << '\n';
  int i = 0;
  for (const auto& prob : model.probs) {
    os << "\t" << i++ << ": " << prob << "\n";
  }
  return os;
}

class CountingFeatureModel : public AttrConstraintModel {
public:

  CountingFeatureModel(const std::string& name,
                       const Feature& f,
                const std::vector<std::string>& counter_names,
                const std::vector<int>& property_to_counter)
      : AttrConstraintModel(name), counters_(counter_names.size()), property_to_counter(property_to_counter), f(f) {
    for (size_t i = 0; i < counter_names.size(); i++) {
      counters_[i].name = counter_names[i];
    }
  }

  void AddAttr(const Attribute& attr, const std::vector<View>& views) {
    CHECK_LT(static_cast<int>(attr.type), property_to_counter.size());
    Feature::Value value = f.ValueAttr(attr, views);
    counters_[property_to_counter[static_cast<int>(attr.type)]].Add(value);
  }

  double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    return AttrProbInner(f.ValueAttr(attr, views), attr.type);
  }

  Feature::Value AttrValue(const Attribute& attr, const std::vector<View>& views) const {
    return f.ValueAttr(attr, views);
  }

  friend std::ostream& operator<<(std::ostream& os, const CountingFeatureModel& node);

  void Dump(std::ostream& os) const {
    os << *this;
  }

  void SaveOrDie(FILE* file) const {
    Serialize::WriteVectorClass(counters_, file);
    Serialize::WriteVector(property_to_counter, file);
  }

  void LoadOrDie(FILE* file) {
    Serialize::ReadVectorClass(&counters_, file);
    Serialize::ReadVector(&property_to_counter, file);
  }

private:
  double AttrProbInner(Feature::Value value, const ConstraintType& type) const {
    const ValueCounter<Feature::Value>& counter = counters_[property_to_counter[static_cast<int>(type)]];
    return ((double) counter.GetCount(value) + 1.0) / (counter.UniqueValues() + counter.TotalCount());
  }

protected:
  std::vector<ValueCounter<Feature::Value>> counters_;
  std::vector<int> property_to_counter;
  const Feature f;
};


inline std::ostream& operator<<(std::ostream& os, const CountingFeatureModel& model) {
  os << model.name << '\n';
  for (const auto& counter : model.counters_) {
    os << "\t" << counter.name << ": total_count(" << counter.TotalCount() << ")\n";
    counter.MostCommon(10, [&os](Feature::Value value, int count){
      os << "\t\t" << count << ": " << value.first << ", " << value.second << "\n";
    });
    if (counter.UniqueValues() > 10) {
      os << "\t\t" << (counter.UniqueValues() - 10) << " more values...\n";
    }

  }
  return os;
}

namespace ConstraintModel {
  std::unique_ptr<AttrConstraintModel> GetMarginModel();

  std::unique_ptr<AttrConstraintModel> GetOrientationModel();

  std::unique_ptr<AttrConstraintModel> GetDistanceModel();

  std::unique_ptr<AttrConstraintModel> GetTypeModel();

  std::unique_ptr<AttrConstraintModel> GetIntersectionModel();

  std::unique_ptr<AttrConstraintModel> GetViewSizeModel();
  std::unique_ptr<AttrConstraintModel> GetViewSizeNameModel();
  std::unique_ptr<AttrConstraintModel> GetViewSizeDimensionRatioModel();
}


class ViewSizeModelWrapper : public ProbModel {
public:
  ViewSizeModelWrapper();

  void AddModel(std::unique_ptr<AttrConstraintModel>&& model, double weight) {
    models.push_back(std::move(model));
    weights.push_back(weight);
  }

  void Train(const std::string& data_path);

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const {
    std::string s;
    for (size_t i = 0; i < models.size(); i++) {
      double prob = models[i]->AttrProb(attr, views);
      StringAppendF(&s, "\t\t%f %f weight=%.1f: %s\n", prob, std::log(prob), weights[i], models[i]->DebugProb(attr, views).c_str());
    }
    StringAppendF(&s, "\t\ttotal: %f\n", AttrProb(attr, views));
    return s;
  }

  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    double res = 0;
    for (size_t i = 0; i < models.size(); i++) {
      double prob = models[i]->AttrProb(attr, views);
      res += std::log(prob) * weights[i];
    }
    return res;
  }

  void AddAttr(const Attribute& attr, const std::vector<View>& views) {
    for (const auto& model : models) {
      model->AddAttr(attr, views);
    }
  }

  void Dump() const {
    for (const auto& model : models) {
      model->Dump(LOG(INFO));
      LOG(INFO) << "\n";
    }
  }


private:
  std::vector<std::unique_ptr<AttrConstraintModel>> models;
  std::vector<double> weights;
};




class ConstraintModelWrapper : public ProbModel {
public:

  ConstraintModelWrapper();

  void AddModel(std::unique_ptr<AttrConstraintModel>&& model, double weight) {
    models.push_back(std::move(model));
    weights.push_back(weight);
  }

  void Train(const std::string& data_path);

  virtual std::string DebugProb(const Attribute& attr, const std::vector<View>& views) const {
    std::string s;
    for (size_t i = 0; i < models.size(); i++) {
      double prob = models[i]->AttrProb(attr, views);
      StringAppendF(&s, "\t\t%f %f weight=%.1f: %s\n", prob, std::log(prob), weights[i], models[i]->DebugProb(attr, views).c_str());
    }
    StringAppendF(&s, "\t\ttotal: %f\n", AttrProb(attr, views));
    return s;
  }

  virtual double AttrProb(const Attribute& attr, const std::vector<View>& views) const {
    double res = 0;
    for (size_t i = 0; i < models.size(); i++) {
      double prob = models[i]->AttrProb(attr, views);
      res += std::log(prob) * weights[i];
    }
    return res;
  }

  void AddAttr(const Attribute& attr, const std::vector<View>& views) {
    for (const auto& model : models) {
      model->AddAttr(attr, views);
    }
  }

  void SaveOrDie(const std::string& file_path) const {
    CHECK(FileExists(file_path.c_str()));
    FILE *file = fopen(file_path.c_str(), "wb");
    SaveOrDie(file);
    fclose(file);
  }

  void SaveOrDie(FILE* file) const {
    for (const auto& model : models) {
      model->SaveOrDie(file);
    }
  }

  void LoadOrDie(const std::string& file_path) {
    CHECK(FileExists(file_path.c_str()));
    FILE *file = fopen(file_path.c_str(), "rb");
    LoadOrDie(file);
    fclose(file);
  }

  void LoadOrDie(FILE* file) {
    for (const auto& model : models) {
      model->LoadOrDie(file);
    }
  }

  void Dump() const {
    for (const auto& model : models) {
      model->Dump(LOG(INFO));
      LOG(INFO) << "\n";
    }
  }


private:
  std::vector<std::unique_ptr<AttrConstraintModel>> models;
  std::vector<double> weights;
};

#endif //CC_SYNTHESIS_CONSTRAINT_MODEL_H
