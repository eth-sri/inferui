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

#ifndef CC_SYNTHESIS_UTIL_H
#define CC_SYNTHESIS_UTIL_H

#include <regex>

#include "inferui/model/util/constants.h"
#include "inferui/model/uidump.pb.h"
#include "base/counter.h"
#include "util/recordio/recordio.h"



enum class Orientation {
  HORIZONTAL = 0,
  VERTICAL
};


template <class T>
class OrientationContainer {
  std::vector<T> values;
public:

  OrientationContainer(T&& first, T&& second) {
    values.push_back(first);
    values.push_back(second);
  }

  T& operator[](const Orientation& orientation) {
    return values[static_cast<int>(orientation)];
  }

  const T& operator[](const Orientation& orientation) const {
    return values[static_cast<int>(orientation)];
  }
};

struct Padding {
public:
  Padding() : paddingLeft(0), paddingRight(0), paddingTop(0), paddingBottom(0) {
  }

  void Initialize(const ProtoView& view);

  void ToProperties(const Constants::Type& output_type, std::unordered_map<std::string, std::string>& properties) const {
    if (paddingLeft != 0) {
      properties[Constants::name(Constants::paddingLeft, output_type)] = StringPrintf("%dpx", paddingLeft);
    }
    if (paddingRight != 0) {
      properties[Constants::name(Constants::paddingRight, output_type)] = StringPrintf("%dpx", paddingRight);
    }
    if (paddingTop != 0) {
      properties[Constants::name(Constants::paddingTop, output_type)] = StringPrintf("%dpx", paddingTop);
    }
    if (paddingBottom != 0) {
      properties[Constants::name(Constants::paddingBottom, output_type)] = StringPrintf("%dpx", paddingBottom);
    }
  }

  int paddingLeft;
  int paddingRight;
  int paddingTop;
  int paddingBottom;
};

std::string OrientationStr(const Orientation& cmd);

inline std::ostream& operator<<(std::ostream& os, const Orientation& size) {
  os << OrientationStr(size);
  return os;
}

namespace std {
  template <> struct hash<Orientation> {
    size_t operator()(const Orientation& x) const {
      return static_cast<int>(x);
    }
  };
}

enum class ViewSize {
  MATCH_PARENT = 0,
  MATCH_CONSTRAINT,
  FIXED,
};

std::string ViewSizeStr(const ViewSize& size);
std::string ViewSizeStr(const ViewSize& size, int value);

inline std::ostream& operator<<(std::ostream& os, const ViewSize& size) {
  os << ViewSizeStr(size);
  return os;
}

ViewSize GetViewSize(const std::string& name);
ViewSize GetViewSize(const ProtoView& view, Orientation orientation);

bool ValidApp(const ProtoScreen& app, ValueCounter<std::string>* stats = nullptr);

int CountProperties(const ProtoView& view, const std::vector<Constants::Name>& properties);

bool HasConstraintBias(const ProtoScreen& app);
bool HasRelativeConstraint(const ProtoView& view);
bool HasRelativeParentConstraint(const ProtoView& view);
std::unordered_set<std::string> GetReferencedIds(const ProtoScreen& app);

bool InRootLayout(const ProtoScreen& app, const ProtoView& view, Constants::Name layout_type);
bool InRootConstraintLayout(const ProtoScreen& app, const ProtoView& view);

std::string PrintLayout(const ProtoScreen& app);

const ProtoView* FindViewById(
    const ProtoScreen& app,
    const ProtoView& view,
    const std::string& id
);


struct ConstraintStats {
  int total;
  int unresolved;
  int ignored;
  int total_apps;
  int unresolved_apps;
  int ignored_apps;

  ConstraintStats() : total(0), unresolved(0), ignored(0), total_apps(0), unresolved_apps(0), ignored_apps(0) {
  }

  void Merge(const ConstraintStats& other);

  std::string ToString() const;
};

const ProtoView* FindPropertyTarget(const ProtoScreen& app, const ProtoView& view, const Constants::Name& property_name);
std::pair<const ProtoView*, Constants::Name> FindPropertyTarget(const ProtoScreen& app, const ProtoView& view, const std::vector<Constants::Name>& properties);

ConstraintStats ForEachConstraint(
    const ProtoScreen& app,
    Constants::Name type,
    const std::function<void(const Constants::Name&, const ProtoView&, const ProtoView&)>& cb
);

void ForEachConstraintLayoutConstraint(
    const ProtoScreen& app,
    const std::function<void(const ProtoView&, std::pair<const ProtoView*, Constants::Name>&, std::pair<const ProtoView*, Constants::Name>&, const Orientation&)>& cb);

class ValueParser {
public:

  static bool hasValue(const std::string& input);
  static float parseValue(const std::string& input);

  static std::string parseID(const std::string& input);
  static int parseViewSeqID(const std::string& input);

  static bool hasPxValue(const std::string& input);
  static int parsePxValue(const std::string& input);

private:
  static const std::vector<std::regex> value_regex;
  static const std::regex id_regex;
  static const std::regex view_id_regex;
  static const std::regex px_regex;
};

float GetMarginFromProto(const ProtoView& proto_view, const Constants::Name& type);
float GetBiasFromProto(const ProtoView& proto_view, const Orientation& orientation);

struct Device {
public:
  Device(int width, int height) : width(width), height(height) {

  }

  int width;
  int height;
};

inline std::ostream& operator<<(std::ostream& os, const Device& device) {
  os << "(width=" << device.width << ", height=" << device.height << ")";
  return os;
}

bool ViewsInsideScreen(const ProtoScreen& app);


#endif //CC_SYNTHESIS_UTIL_H
