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

#ifndef CC_SYNTHESIS_ITERATORUTIL_H
#define CC_SYNTHESIS_ITERATORUTIL_H


#include <iterator>
#include <vector>
#include <glog/logging.h>

template <class T>
class MultiSortedIterator {
  using Values = std::vector<T>;
public:

  MultiSortedIterator(const std::vector<Values>& values, bool end = false) : values_(values) {
    CHECK_GE(values_.size(), 1);

    if (end) {
      current_ = 0;
      top_.emplace_back(values[0].size());
    } else {
      top_.assign(values_.size(), 0);
      SetNext();
    }
  }

  void Reset() {
    top_.assign(values_.size(), 0);
    SetNext();
  }

  void SetCurrentToEnd() {
    top_[current_] = values_[current_].size() - 1;
  }

  // Iterator traits, previously from std::iterator.
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_reference = const T&;
  using iterator_category = std::forward_iterator_tag;

  // Default constructible.
  MultiSortedIterator() = default;
//  explicit MultiSortedIterator(T* node);

  // Dereferencable.
  const_reference operator*() const {
    return values_[current_][top_[current_]];
  }

  // Pre- and post-incrementable.
  MultiSortedIterator<T>& operator++() {
    top_[current_]++;
    SetNext();
    return *this;
  }

  // Post-incrementable: it++.
  MultiSortedIterator<T> operator++(int) {
    MultiSortedIterator<T> tmp = *this;
    top_[current_]++;
    SetNext();
    return tmp;
  }

  // Equality / inequality.
  bool operator==(const MultiSortedIterator<T>& rhs) {
    return current_ == rhs.current_ && top_[current_] == rhs.top_[rhs.current_];
  }

  bool operator!=(const MultiSortedIterator<T>& rhs) {
    return !(*this == rhs);
  }

private:

  void SetNext() {
    current_ = 0;

    bool first = true;
    for (size_t i = 0; i < values_.size(); i++) {
      if (top_[i] == values_[i].size()) continue;

      if (first || values_[i][top_[i]] > values_[current_][top_[current_]]) {
        current_ = i;
        first = false;
      }
    }
  }

  const std::vector<Values>& values_;

  std::vector<typename Values::size_type> top_;
  typename Values::size_type current_;
};


#endif //CC_SYNTHESIS_ITERATORUTIL_H
