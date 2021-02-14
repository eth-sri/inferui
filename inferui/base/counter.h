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

#ifndef CC_SYNTHESIS_COUNTER_H
#define CC_SYNTHESIS_COUNTER_H


#include <unordered_map>
#include <vector>
#include <functional>
#include <map>
#include <iomanip>
#include "base/maputil.h"
#include "base/serializeutil.h"
#include "base/stringprintf.h"

template<class V>
class ValueCounter {
public:
  ValueCounter() : total_count_(0) {}

  ValueCounter<V> operator+ (const ValueCounter<V>& c) const {
    ValueCounter<V> result;
    for (const auto& entry : this->data_) {
      result.Add(entry.first, entry.second);
    }
    for (const auto& entry : c.data_) {
      result.Add(entry.first, entry.second);
    }
    return result;
  }


  void Add(const V& value, int count = 1) {
    data_[value] += count;
    total_count_ += count;
  }

  int TotalCount() const {
    return total_count_;
  }

  int GetCount(V key) const {
    return FindWithDefault(data_, key, 0);
  }

  size_t UniqueValues() const {
    return data_.size();
  }

  void MostCommon(size_t n, const std::function<void(V, int)>& cb) const {
    const std::vector<std::pair<int, V>>& sorted_values = SortMap(data_);
    for (size_t i = 0; i < n && i < sorted_values.size(); i++) {
      cb(sorted_values[i].second, sorted_values[i].first);
    }
  }

  void SaveOrDie(FILE* file) const {
    Serialize::Write(total_count_, file);
    Serialize::Write(name, file);
    Serialize::WriteMap(data_, file);
  }

  void LoadOrDie(FILE* file) {
    Serialize::Read(&total_count_, file);
    Serialize::Read(&name, file);
    Serialize::ReadMap(&data_, file);
  }

  template<class T>
  friend std::ostream& operator<<(std::ostream& os, const ValueCounter<T>& node);


  std::string name;
private:
  std::unordered_map<V, int> data_;
  int total_count_;
};


template <class V>
inline std::ostream& operator<<(std::ostream& os, const ValueCounter<V>& counter) {
  os << counter.name << ": total_count(" << counter.TotalCount() << ")\n";
  counter.MostCommon(16, [&os](V value, int count){
    os << "\t" << count << ": " << value << "\n";
  });
  if (counter.UniqueValues() > 16) {
    os << "\t" << (counter.UniqueValues() - 16) << " more values...\n";
  }
  return os;
}


template <class V>
class ConfusionMatrixCounter {
public:

  ConfusionMatrixCounter(const std::string& name) : total_(0), correct_(0), name_(name) {

  }

  void Add(const V& expected, const V& actual) {
    counts_[expected][actual]++;
    total_++;
    if (expected == actual) correct_++;
  }

  std::string Accuracy(const V& expected, const V& actual) const {
    if (!Contains(counts_, expected)) return StringPrintf("- (-%%)");

    int value = FindWithDefault(counts_.at(expected), actual, 0);
    return StringPrintf("%d (%.2f%%)", value, (value * 100.0) / total_);
  }

  std::set<V> GetVocab() const {
    std::set<V> vocab;
    for (const auto& it : counts_) {
      vocab.insert(it.first);
      for (const auto& v : it.second) {
        vocab.insert(v.first);
      }
    }
    return vocab;
  }

  template <class T>
  friend std::ostream& operator<<(std::ostream& os, const ConfusionMatrixCounter<T>& attr);

private:
  std::map<V, std::map<V, int>> counts_;
  int total_;
  int correct_;
  const std::string name_;
};

template <class V>
inline std::ostream& operator<<(std::ostream& os, const ConfusionMatrixCounter<V>& counter) {
  os << counter.name_ << ": total_count(" << counter.total_ << "), correct: " << counter.correct_ << " (" << ((counter.correct_ * 100.0) / counter.total_) << "%)\n";

  std::set<V> vocab = counter.GetVocab();

  // Header
  os << std::setfill(' ') << std::setw(16) << "predicted/actual";
  for (const V& v : vocab) {
    os << '\t' << std::setfill(' ') << std::setw(16) << v;
  }
  os << '\n';

  // Values
  for (const V& v : vocab) {
    os << std::setfill(' ') << std::setw(16) << v << ":";
    for (const V& k : vocab) {
      os << '\t' << std::setfill(' ') << std::setw(16) << counter.Accuracy(k, v);
    }
    os << '\n';
  }

  return os;
}

#endif //CC_SYNTHESIS_COUNTER_H
