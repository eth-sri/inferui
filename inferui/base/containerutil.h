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

#ifndef CC_SYNTHESIS_CONTAINERUTIL_H
#define CC_SYNTHESIS_CONTAINERUTIL_H

#include <vector>

template <class V, class Callback>
void Product(std::vector<V>& items, const Callback& cb) {
  for (V& it1 : items) {
    for (V& it2 : items) {
      cb(it1, it2);
    }
  }
};

template <class V, class Callback>
void Product(const std::vector<V>& items, const Callback& cb) {
  for (const V& it1 : items) {
    for (const V& it2 : items) {
      cb(it1, it2);
    }
  }
};

template <class Value>
bool Contains(const std::vector<Value>& c, const Value& key) {
  return std::find(c.begin(), c.end(), key) != c.end();
}

#endif //CC_SYNTHESIS_CONTAINERUTIL_H
