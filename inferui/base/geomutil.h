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

#ifndef CC_SYNTHESIS_GEOMUTIL_H
#define CC_SYNTHESIS_GEOMUTIL_H

#include <math.h>
#include <cmath>
#include <algorithm>
#include <array>
#include <glog/logging.h>

//# If F(x y) = 0, (x y) is ON the line.
//# If F(x y) > 0, (x y) is "above" the line.
//# If F(x y) < 0, (x y) is "below" the line.
int F(int x, int y, int x1, int x2, int y1, int y2);

template <class Rectangle>
bool intersects_loose(int xleft, int ytop, int xright, int ybottom, const Rectangle& node) {

//# make sure that the line segment is properly oriented
  if (xleft > xright) {
    std::swap(xleft, xright);
  }

  if (ybottom < ytop) {
    std::swap(ytop, ybottom);
  }

//# If (x1 > xTR and x2 > xTR), no intersection (line is to right of rectangle).
  if (xleft >= node.xright && xright >= node.xright) return false;

//# If (x1 < xBL and x2 < xBL), no intersection (line is to left of rectangle).
  if (xleft <= node.xleft && xright <= node.xleft) return false;

//# If (y1 > yTR and y2 > yTR), no intersection (line is above rectangle).
  if (ytop <= node.ytop && ybottom <= node.ytop) return false;

//# If (y1 < yBL and y2 < yBL), no intersection (line is below rectangle).
  if (ytop >= node.ybottom && ybottom >= node.ybottom) return false;

//# segment completely inside
  if (node.ytop <= ytop && node.ybottom >= ybottom && node.xleft <= xleft && node.xright >= xright) {
    return false;
  }

  std::array<int, 4> sides = {
      F(node.xleft, node.ybottom, xleft, xright, ytop, ybottom),
      F(node.xleft, node.ytop, xleft, xright, ytop, ybottom),
      F(node.xright, node.ybottom, xleft, xright, ytop, ybottom),
      F(node.xright, node.ytop, xleft, xright, ytop, ybottom)
  };

  if (std::all_of(sides.begin(), sides.end(), [](int i){return i >= 0;})) {
    return false;
  }

  if (std::all_of(sides.begin(), sides.end(), [](int i){return i <= 0;})) {
    return false;
  }

  return true;
};

template <class Rectangle>
bool intersects(int xleft, int ytop, int xright, int ybottom, const Rectangle& node) {

//# make sure that the line segment is properly oriented
  if (xleft > xright) {
    std::swap(xleft, xright);
  }

  if (ybottom < ytop) {
    std::swap(ytop, ybottom);
  }

//# If (x1 > xTR and x2 > xTR), no intersection (line is to right of rectangle).
  if (xleft > node.xright && xright > node.xright) return false;

//# If (x1 < xBL and x2 < xBL), no intersection (line is to left of rectangle).
  if (xleft < node.xleft && xright < node.xleft) return false;

//# If (y1 > yTR and y2 > yTR), no intersection (line is above rectangle).
  if (ytop < node.ytop && ybottom < node.ytop) return false;

//# If (y1 < yBL and y2 < yBL), no intersection (line is below rectangle).
  if (ytop > node.ybottom && ybottom > node.ybottom) return false;

//# segment completely inside
  if (node.ytop < ytop && node.ybottom > ybottom && node.xleft < xleft && node.xright > xright) {
    return false;
  }

  std::array<int, 4> sides = {
      F(node.xleft, node.ybottom, xleft, xright, ytop, ybottom),
      F(node.xleft, node.ytop, xleft, xright, ytop, ybottom),
      F(node.xright, node.ybottom, xleft, xright, ytop, ybottom),
      F(node.xright, node.ytop, xleft, xright, ytop, ybottom)
  };

  if (std::all_of(sides.begin(), sides.end(), [](int i){return i >= 0;})) {
    return false;
  }

  if (std::all_of(sides.begin(), sides.end(), [](int i){return i <= 0;})) {
    return false;
  }

  return true;
};

std::pair<int, int> ClosestPoint(int xleft, int xright, int yleft, int yright);
std::pair<int, int> ClosestPointIntersection(int xleft, int xright, int yleft, int yright);

struct LineSegment {
public:
  LineSegment(int left, int top, int right, int bottom) : xleft(left), xright(right), ytop(top), ybottom(bottom) {
  }

  float GetAngle() const {
    float xdelta = xright - xleft;
    float ydelta = ybottom - ytop;

    if (xdelta == 0 && ydelta == 0) {
      return NAN;
    }

    float theta_radians = std::atan2(ydelta, xdelta);
    return theta_radians * (180.0 / M_PI);
  }

  float Length() const {
    float xdelta = xright - xleft;
    float ydelta = ybottom - ytop;

    return std::sqrt(xdelta*xdelta + ydelta*ydelta);;
  }

  template <class Rectangle>
  bool IntersectsLoose(const Rectangle& node) const {
    return intersects_loose(xleft, ytop, xright, ybottom, node);
  }

  template <class Rectangle>
  bool Intersects(const Rectangle& node) const {
    return intersects(xleft, ytop, xright, ybottom, node);
  }

  const int xleft;
  const int xright;
  const int ytop;
  const int ybottom;
};



#endif //CC_SYNTHESIS_GEOMUTIL_H
