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
#include <cstdlib>
#include "hir2mpl_ut_environment.h"
#define private public
#include "bc_compiler_component.h"
#undef private
#include "dexfile_factory.h"
#include "dex_pragma.h"

namespace maple {
TEST(BCLoadOnDemandType, Test) {
  std::string str(getenv("MAPLE_ROOT"));
  std::string path = str + "/output/aarch64-clang-release/libjava-core/host-x86_64-O2/libcore-all.dex";
  FEOptions::GetInstance().SetXBootClassPath(path);
  bc::BCCompilerComponent<bc::DexReader> bcCompiler(HIR2MPLUTEnvironment::GetMIRModule());
  std::unordered_set<std::string> allDepSet;
  std::list<std::unique_ptr<bc::BCClass>> klassList;
  allDepSet.insert("Ljava/lang/String;");
  std::unordered_set<std::string> allClassSet;
  bool success = bcCompiler.LoadOnDemandType2BCClass(allDepSet, allClassSet, klassList);
  EXPECT_EQ(success, true);
  EXPECT_EQ(klassList.size(), 5);
  ASSERT_EQ(klassList.front()->GetClassName(false), "Ljava/lang/Object;");
  for (const std::unique_ptr<bc::BCClass> &klass : klassList) {
    // Dependent classes were loaded by mplt, rename class names to avoid MIRStruct types are created failed
    klass->SetClassName("Temp/" + klass->GetClassName(false));
    // Annotations are set nullptr instead of parsed failed,
    // because above renamed classname is different from klass in local orinNameStrIdxMap.
    DexFileFactory dexFileFactory;
    std::unique_ptr<maple::IDexFile> iDexFile = dexFileFactory.NewInstance();
    std::unique_ptr<bc::BCAnnotationsDirectory> annotationsDirectory = std::make_unique<bc::DexBCAnnotationsDirectory>(
        HIR2MPLUTEnvironment::GetMIRModule(), *(HIR2MPLUTEnvironment::GetMIRModule().GetMemPool()), *iDexFile, "", nullptr);
    klass->SetAnnotationsDirectory(std::move(annotationsDirectory));
  }
  std::list<std::unique_ptr<FEInputStructHelper>> structHelpers;
  success = bcCompiler.LoadOnDemandBCClass2FEClass(klassList, structHelpers, false);
  EXPECT_EQ(structHelpers.size(), 5);
  ASSERT_EQ(structHelpers.front()->GetStructNameOrin(), "Temp/Ljava/lang/Object;");
  ASSERT_EQ(structHelpers.front()->GetContainer()->IsImported(), true);
  ASSERT_EQ(structHelpers.front()->GetMethodHelpers().front()->GetMethodName(false, false), "<init>");
}
}  // namespace maple
