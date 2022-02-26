/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef MAPLEBE_INCLUDE_STANDARDIZE_H
#define MAPLEBE_INCLUDE_STANDARDIZE_H

#include "cgfunc.h"
namespace maplebe {
class Standardize {
 public:
  explicit Standardize(CGFunc &f) : cgFunc(&f) {}

  virtual ~Standardize() = default;

  /*
   * for cpu instruction contains different operands
   * maple provide a default implement from three address to two address
   */
  void TwoAddressMapping();
  void DoStandardize();
 private:
  CGFunc *cgFunc;
};
}
#endif  /* MAPLEBE_INCLUDE_STANDARDIZE_H */
