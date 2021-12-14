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

//////////////////////////////////////////////////////////////////////////////////////////////
//                This is the interface to translate AST to MapleIR.
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AST2MPL_HEADER__
#define __AST2MPL_HEADER__

#include "astopt.h"
#include "ast_handler.h"

#include "mir_module.h"
#include "maplefe_mir_builder.h"

namespace maplefe {

class A2M : public AstOpt {
private:
  AST_Handler *mASTHandler;
  unsigned     mFlags;
  unsigned     mIndexImported;

public:
  explicit A2M(AST_Handler *h, unsigned flags) :
    AstOpt(h, flags),
    mASTHandler(h),
    mFlags(flags),
    mIndexImported(0) {}
  ~A2M() = default;

  // return 0 if successful
  // return non-zero if failed
  int ProcessAST();
};

}
#endif
