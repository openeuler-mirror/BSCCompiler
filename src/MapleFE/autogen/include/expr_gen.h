/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*
*  http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/
////////////////////////////////////////////////////////////////////////
//                     Expr Generation                                //
////////////////////////////////////////////////////////////////////////

#ifndef __EXPR_GEN_H__
#define __EXPR_GEN_H__

#include "base_gen.h"
#include "all_supported.h"

namespace maplefe {

class ExprGen : public BaseGen {
public:
  ExprGen(const char *dfile, const char *hfile, const char *cfile)
      : BaseGen(dfile, hfile, cfile) {}
  ~ExprGen(){}

  void Generate();
  void GenCppFile();
  void GenHeaderFile();
};

}

#endif
