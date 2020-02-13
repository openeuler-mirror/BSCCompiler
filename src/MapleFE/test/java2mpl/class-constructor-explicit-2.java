//
//Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
//
//OpenArkFE is licensed under the Mulan PSL v1.
//You can use this software according to the terms and conditions of the Mulan PSL v1.
//You may obtain a copy of Mulan PSL v1 at:
//
// http://license.coscl.org.cn/MulanPSL
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
//FIT FOR A PARTICULAR PURPOSE.
//See the Mulan PSL v1 for more details.
//
class Point {
  int x, y;
  Point(int x, int y) { this.x = x; this.y = y; }
}
class ColoredPoint extends Point {
  static final int WHITE = 0, BLACK = 1;
  int color;
  ColoredPoint(int x, int y) {
    this(x, y, color);
  }
  ColoredPoint(int x, int y, int color) {
    super(x, y);
    this.color = color;
  }
}
