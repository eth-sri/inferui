/*
   Copyright 2013 Software Reliability Lab, ETH Zurich

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

#ifndef BASE_H_
#define BASE_H_

#include <stddef.h>
#include <string>
#include <stack>
#include <map>
#include <glog/logging.h>
#include <cmath>

typedef long long int64;
typedef unsigned long long uint64;

int64 GetCurrentTimeMicros();

class Timer {
public:

  void Start() {
    time_ = GetCurrentTimeMicros();
  }

  int64 Stop() {
    return GetCurrentTimeMicros() - time_;
  }

  void StartScope(const std::string& name) {
    scopes.push(name);
    scope_starts.push(GetCurrentTimeMicros());
  }

  void EndScope() {
    std::string name = std::string(scopes.top().c_str());
    int64 start = scope_starts.top();
    scopes.pop();
    scope_starts.pop();
    runtimes[name] += GetCurrentTimeMicros() - start;
  }

  double GetMilliSeconds() {
    return ((double)(GetCurrentTimeMicros() - time_)) / 1000.0;
  }

  void Dump() {
    int64 total = 0;
    for (const auto& entry : runtimes) {
      total += entry.second;
    }
    for (const auto& entry : runtimes) {
      LOG(INFO) << entry.first << ": " << std::round(entry.second / 1000.0) << "ms (" << std::round(entry.second * 100.0 / total) << "%)";
    }
  }

private:
  int64 time_;
  std::stack<std::string> scopes;
  std::stack<int64> scope_starts;
  std::map<std::string, int64> runtimes;
};

inline unsigned FingerprintCat(unsigned a, unsigned b) {
  return a * 6037 + ((b * 17) ^ (b >> 16));
}

inline uint64 FingerprintCat64(uint64 a, uint64 b) {
  return a * 6037 + ((b * 17) ^ (b >> 16));
}

inline size_t FingerprintMem(const void* memory, unsigned size) {
  size /= sizeof(uint64);
  const uint64* mem = static_cast<const uint64*>(memory);
  size_t r = 0;
  for (unsigned i = 0; i < size; ++i) {
    uint64 tmp = mem[i];
    r = r * 6037 + ((tmp * 19) ^ (tmp >> 48));
  }
  return r;
}

template <class V>
inline bool EqualOrNull(const V* a, const V* b) {
  return (a == nullptr && b == nullptr) || (a != nullptr && b != nullptr && *a == *b);
}

#endif /* BASE_H_ */
