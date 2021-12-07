/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "feir_var_reg.h"
#include "feir_var_name.h"
#include "feir_type_helper.h"
#include "hir2mpl_ut_regx.h"
#include "feir_builder.h"

namespace maple {
class FEIRVarTest : public FEIRTestBase {
 public:
  FEIRVarTest() = default;
  virtual ~FEIRVarTest() = default;
};

TEST_F(FEIRVarTest, FEIRVarReg) {
  std::unique_ptr<FEIRType> type = FEIRTypeHelper::CreateTypeByJavaName("Ltest/lang/Object;", false, true);
  FEIRVarReg varReg(1, std::move(type));
  varReg.SetGlobal(false);
  MIRSymbol *symbol = varReg.GenerateMIRSymbol(mirBuilder);
  RedirectCout();
  std::string symbolName = symbol->GetName();
  std::string strPattern = HIR2MPLUTRegx::RegName(1) + "_" + HIR2MPLUTRegx::RefIndex(HIR2MPLUTRegx::kAnyNumber);
  EXPECT_EQ(HIR2MPLUTRegx::Match(symbolName, strPattern), true);
  symbol->Dump(true, 0);
  std::string symbolDump = GetBufferString();
  std::string strPattern2 = "var %" + HIR2MPLUTRegx::RegName(1) + "_" + HIR2MPLUTRegx::RefIndex(HIR2MPLUTRegx::kAnyNumber) +
                            HIR2MPLUTRegx::Any();
  EXPECT_EQ(HIR2MPLUTRegx::Match(symbolDump, strPattern2), true);
  RestoreCout();
}

TEST_F(FEIRVarTest, FEIRVarName) {
  std::unique_ptr<FEIRType> type = FEIRTypeHelper::CreateTypeByJavaName("Ltest/lang/Object;", false, true);
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("Ltest_2Flang_2FObject_3B_7Cklass");
  FEIRVarName varName(nameIdx, std::move(type));
  varName.SetGlobal(true);
  MIRSymbol *symbol = varName.GenerateMIRSymbol(mirBuilder);
  RedirectCout();
  std::string symbolName = symbol->GetName();
  EXPECT_EQ(symbolName, "Ltest_2Flang_2FObject_3B_7Cklass");
  symbol->Dump(false, 0);
  std::string symbolDump = GetBufferString();
  std::string strPattern2 = std::string("var \\$") + "Ltest_2Flang_2FObject_3B_7Cklass" + HIR2MPLUTRegx::Any();
  EXPECT_EQ(HIR2MPLUTRegx::Match(symbolDump, strPattern2), true);
  RestoreCout();
}

TEST_F(FEIRVarTest, FEIRVarTransDirect) {
  UniqueFEIRVar var = FEIRBuilder::CreateVarReg(0, PTY_ref, false);
  FEIRVarTrans trans(FEIRVarTransKind::kFEIRVarTransDirect, var);
  RedirectCout();
  UniqueFEIRType type1 = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/Object;", false, false);
  UniqueFEIRType type1T = trans.GetType(type1);
  type1T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<$Ljava_2Flang_2FObject_3B>");
  ClearBufferString();

  UniqueFEIRType type2 = FEIRTypeHelper::CreateTypeByJavaName("Ltest/lang/Object;", false, true);
  UniqueFEIRType type2T = trans.GetType(type2);
  type2T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <$Ltest_2Flang_2FObject_3B>>");
  ClearBufferString();

  UniqueFEIRType type3 = FEIRTypeHelper::CreateTypeByJavaName("I", false, false);
  UniqueFEIRType type3T = trans.GetType(type3);
  type3T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "i32");
  ClearBufferString();

  UniqueFEIRType type4 = FEIRTypeHelper::CreateTypeByJavaName("[I", false, true);
  UniqueFEIRType type4T = trans.GetType(type4);
  type4T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] i32>>");
  ClearBufferString();
  RestoreCout();
}

TEST_F(FEIRVarTest, FEIRVarTransArrayDimIncr) {
  UniqueFEIRVar var = FEIRBuilder::CreateVarReg(0, PTY_ref, false);
  FEIRVarTrans trans(FEIRVarTransKind::kFEIRVarTransArrayDimIncr, var);
  RedirectCout();
  UniqueFEIRType type1 = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/Object;", false, false);
  UniqueFEIRType type1T = trans.GetType(type1);
  UniqueFEIRType type1T2 = trans.GetType(type1, PTY_ref, false);
  type1T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <$Ljava_2Flang_2FObject_3B>>>>");
  ClearBufferString();
  type1T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<[] <* <$Ljava_2Flang_2FObject_3B>>>");
  ClearBufferString();

  UniqueFEIRType type2 = FEIRTypeHelper::CreateTypeByJavaName("[Ljava/lang/Object;", false, true);
  UniqueFEIRType type2T = trans.GetType(type2);
  UniqueFEIRType type2T2 = trans.GetType(type2, PTY_ref, false);
  type2T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <[] <* <$Ljava_2Flang_2FObject_3B>>>>>>");
  ClearBufferString();
  type2T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <[] <* <$Ljava_2Flang_2FObject_3B>>>>>>");
  ClearBufferString();

  UniqueFEIRType type3 = FEIRTypeHelper::CreateTypeByJavaName("I", false, false);
  UniqueFEIRType type3T = trans.GetType(type3);
  UniqueFEIRType type3T2 = trans.GetType(type3, PTY_ref, false);
  type3T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] i32>>");
  ClearBufferString();
  type3T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<[] i32>");
  ClearBufferString();

  UniqueFEIRType type4 = FEIRTypeHelper::CreateTypeByJavaName("[I", false, true);
  UniqueFEIRType type4T = trans.GetType(type4);
  UniqueFEIRType type4T2 = trans.GetType(type4, PTY_ref, false);
  type4T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <[] i32>>>>");
  ClearBufferString();
  type4T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <[] i32>>>>");
  ClearBufferString();
  RestoreCout();
}

TEST_F(FEIRVarTest, FEIRVarTransArrayDimDecr) {
  UniqueFEIRVar var = FEIRBuilder::CreateVarReg(0, PTY_ref, false);
  FEIRVarTrans trans(FEIRVarTransKind::kFEIRVarTransArrayDimDecr, var);
  RedirectCout();
  UniqueFEIRType type1 = FEIRTypeHelper::CreateTypeByJavaName("[Ljava/lang/Object;", false, false);
  UniqueFEIRType type1T = trans.GetType(type1);
  UniqueFEIRType type1T2 = trans.GetType(type1, PTY_ref, false);
  type1T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <$Ljava_2Flang_2FObject_3B>>");
  ClearBufferString();
  type1T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<$Ljava_2Flang_2FObject_3B>");
  ClearBufferString();

  UniqueFEIRType type2 = FEIRTypeHelper::CreateTypeByJavaName("[[Ljava/lang/Object;", false, true);
  UniqueFEIRType type2T = trans.GetType(type2);
  UniqueFEIRType type2T2 = trans.GetType(type2, PTY_ref, false);
  type2T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <$Ljava_2Flang_2FObject_3B>>>>");
  ClearBufferString();
  type2T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] <* <$Ljava_2Flang_2FObject_3B>>>>");
  ClearBufferString();

  UniqueFEIRType type3 = FEIRTypeHelper::CreateTypeByJavaName("[I", false, false);
  UniqueFEIRType type3T = trans.GetType(type3);
  UniqueFEIRType type3T2 = trans.GetType(type3, PTY_ref, false);
  type3T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "i32");
  ClearBufferString();
  type3T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "i32");
  ClearBufferString();

  UniqueFEIRType type4 = FEIRTypeHelper::CreateTypeByJavaName("[[I", false, false);
  UniqueFEIRType type4T = trans.GetType(type4);
  UniqueFEIRType type4T2 = trans.GetType(type4, PTY_ref, false);
  type4T->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<* <[] i32>>");
  ClearBufferString();
  type4T2->GenerateMIRType()->Dump(0);
  EXPECT_EQ(GetBufferString(), "<[] i32>");
  ClearBufferString();
  RestoreCout();
}
}  // namespace maple