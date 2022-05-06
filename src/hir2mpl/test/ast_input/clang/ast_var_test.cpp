/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "feir_test_base.h"
#include "feir_var.h"
#include "feir_var_name.h"
#include "feir_type_helper.h"
#include "hir2mpl_ut_regx.h"
#include "feir_builder.h"
#include "ast_decl_builder.h"

namespace maple {
class FEIRVarNameTest : public FEIRTestBase {
 public:
  FEIRVarNameTest() = default;
  virtual ~FEIRVarNameTest() = default;
};

TEST_F(FEIRVarNameTest, FEIRVarInAST) {
  GenericAttrs attrs;
  attrs.SetAttr(GENATTR_const);
  MIRType *type = GlobalTables::GetTypeTable().GetInt32();
  auto astVar = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("foo.c", mp), "a",
      MapleVector<MIRType*>({type}, allocator.Adapter()), attrs);
  astVar->SetGlobal(false);
  auto feirVar = astVar->Translate2FEIRVar();
  EXPECT_EQ(feirVar->GetKind(), kFEIRVarName);
  EXPECT_EQ(feirVar->GetType()->GetKind(), kFEIRTypeNative);
  EXPECT_EQ(feirVar->GetType()->GenerateMIRTypeAuto(), type);
  EXPECT_EQ(feirVar->GetName(*type).find("a") != std::string::npos, true);  // a_0_0
  EXPECT_EQ(feirVar->GetNameRaw().find("a") != std::string::npos, true);
  EXPECT_EQ(feirVar->GetType()->IsArray(), false);
  MIRSymbol *symbol = feirVar->GenerateMIRSymbol(mirBuilder);
  RedirectCout();
  std::string symbolName = symbol->GetName();
  EXPECT_EQ(symbolName.find("a") != std::string::npos, true);
  EXPECT_EQ(symbol->GetAttr(ATTR_const), true);
  symbol->Dump(false, 0);
  std::string symbolDump = GetBufferString();
  std::string strPattern2 = std::string("var \\$") + "a" + HIR2MPLUTRegx::Any();
  EXPECT_EQ(HIR2MPLUTRegx::Match(symbolDump, strPattern2), true);
  RestoreCout();

  MIRType *type1 = GlobalTables::GetTypeTable().GetInt32();
  auto astVar1 = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("foo.c", mp), "a",
      MapleVector<MIRType*>({type1}, allocator.Adapter()), attrs);
  astVar1->SetGlobal(false);
  auto feirVar1 = astVar1->Translate2FEIRVar();
  EXPECT_EQ(feirVar1->EqualsTo(feirVar), true);

  auto feirVarClone = feirVar->Clone();
  EXPECT_EQ(feirVarClone->EqualsTo(feirVar), true);

  // array type
  uint32 arraySize[3] = {3, 4, 5};
  MIRType *arrayType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*type, 3, arraySize);
  auto astArrVar = ASTDeclsBuilder::ASTVarBuilder(allocator, MapleString("foo.c", mp), "array",
      MapleVector<MIRType*>({arrayType}, allocator.Adapter()), attrs);
  astArrVar->SetGlobal(true);
  auto feirArrVar = astArrVar->Translate2FEIRVar();
  EXPECT_EQ(feirArrVar->GetType()->IsArray(), true);
  MIRSymbol *symbolArr = feirArrVar->GenerateMIRSymbol(mirBuilder);
  RedirectCout();
  symbolName = symbolArr->GetName();
  EXPECT_EQ(symbolName, "array");
  symbolArr->Dump(false, 0);
  symbolDump = GetBufferString();
  strPattern2 = std::string("var \\$") + "array \\<\\[3\\]\\[4\\]\\[5\\] i32\\>" + HIR2MPLUTRegx::Any();
  EXPECT_EQ(HIR2MPLUTRegx::Match(symbolDump, strPattern2), true);
  RestoreCout();
}
}  // namespace maple