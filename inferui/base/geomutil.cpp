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

#include "geomutil.h"

int F(int x, int y, int x1, int x2, int y1, int y2) {
  return (y2 - y1) * x + (x1 - x2) * y + (x2 * y1 - x1 * y2);
}

std::pair<int, int> ClosestPointIntersection(int xleft, int xright, int yleft, int yright) {
  if (xright < yleft) {
    // y completely to the right
    return std::make_pair(xright, yleft);
  } else if (yright < xleft) {
    // y completely to the left
    return std::make_pair(xleft, yright);
  } else {
    // otherwise they intersect
    // return center of intersection

    int left = std::max(xleft, yleft);
    int right = std::min(xright, yright);
    return std::make_pair((left + right) / 2, (left + right) / 2);
  }
};

std::pair<int, int> ClosestPoint(int xleft, int xright, int yleft, int yright) {
  if (xright < yleft) {
    // y completely to the right
    return std::make_pair(xright, yleft);
  } else if (yright < xleft) {
    // y completely to the left
    return std::make_pair(xleft, yright);
  } else {
    // otherwise they intersect

    if (xleft <= yleft && yleft <= xright) {
      return std::make_pair(yleft, yleft);
    } else {
      CHECK(yleft <= xleft && xleft <= yright) << xleft << ", " << xright << " -- " << yleft << ", " << yright;
      return std::make_pair(xleft, xleft);
    }
  }
};