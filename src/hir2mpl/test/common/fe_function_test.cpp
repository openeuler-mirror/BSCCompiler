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
#include "fe_function.h"
#include "redirect_buffer.h"
#include "hir2mpl_ut_environment.h"

namespace maple {
class GeneralStmtAuxPre : public FEIRStmt {
 public:
  GeneralStmtAuxPre() : FEIRStmt(kStmtPesudo) {
    isAuxPre = true;
  }
  ~GeneralStmtAuxPre() = default;
};

class GeneralStmtAuxPost : public FEIRStmt {
 public:
  GeneralStmtAuxPost() : FEIRStmt(kStmtPesudo) {
    isAuxPost = true;
  }
  ~GeneralStmtAuxPost() = default;
};

class FEFunctionDemo : public FEFunction {
 public:
  FEFunctionDemo(MapleAllocator &allocator, MIRFunction &argMIRFunction)
      : FEFunction(argMIRFunction, std::make_unique<FEFunctionPhaseResult>(true)),
        mapIdxStmt(allocator.Adapter()) {}
  ~FEFunctionDemo() = default;

  bool PreProcessTypeNameIdx() override {
    return false;
  }

  bool GenerateGeneralStmt(const std::string &phaseName) override {
    return true;
  }

  void GenerateGeneralStmtFailCallBack() override {}
  void GenerateGeneralDebugInfo() override {}
  bool GenerateArgVarList(const std::string &phaseName) override {
    return true;
  }

  bool HasThis() override {
    return false;
  }

  bool IsNative() override {
    return false;
  }

  bool VerifyGeneral() override {
    return false;
  }

  void VerifyGeneralFailCallBack() override {}
  bool EmitToFEIRStmt(const std::string &phaseName) override {
    return true;
  }

  bool GenerateAliasVars(const std::string &phaseName) override {
    return true;
  }

  void LoadGenStmtDemo1();
  void LoadGenStmtDemo2();
  void LoadGenStmtDemo3();
  void LoadGenStmtDemo4();
  void LoadGenStmtDemo5();

  FEIRStmt *GetStmtByIdx(uint32 idx) {
    CHECK_FATAL(mapIdxStmt.find(idx) != mapIdxStmt.end(), "invalid idx");
    return mapIdxStmt[idx];
  }

  template <typename T>
  T *NewGenStmt(uint32 idx) {
    FEIRStmt *ptrStmt = RegisterGeneralStmt(std::make_unique<T>());
    genStmtTail->InsertBefore(ptrStmt);
    mapIdxStmt[idx] = ptrStmt;
    return static_cast<T*>(ptrStmt);
  }

 private:
  MapleMap<uint32, FEIRStmt*> mapIdxStmt;
};

class FEFunctionTest : public testing::Test, public RedirectBuffer {
 public:
  static MemPool *mp;
  MapleAllocator allocator;
  MIRFunction func;
  FEFunctionDemo demoFunc;
  FEFunctionTest()
      : allocator(mp),
        func(&HIR2MPLUTEnvironment::GetMIRModule(), StIdx(0, 0)),
        demoFunc(allocator, func) {}
  ~FEFunctionTest() = default;

  static void SetUpTestCase() {
    mp = FEUtils::NewMempool("MemPool for FEFunctionTest", false /* isLocalPool */);
  }

  static void TearDownTestCase() {
    delete mp;
    mp = nullptr;
  }
};
MemPool *FEFunctionTest::mp = nullptr;

/* GenStmtDemo1:BB
 * 0   StmtHead
 * 1   Stmt (fallthru = true)
 * 2   Stmt (fallthru = false)
 * 3   StmtTail
 */
void FEFunctionDemo::LoadGenStmtDemo1() {
  Init();
  mapIdxStmt.clear();
  (void)NewGenStmt<FEIRStmt>(1);
  FEIRStmt *stmt2 = NewGenStmt<FEIRStmt>(2);
  stmt2->SetFallThru(false);
}

/* GenStmtDemo2:BB_StmtAux
 * 0   StmtHead
 * 1   StmtAuxPre
 * 2   Stmt (fallthru = true)
 * 3   Stmt (fallthru = false)
 * 4   StmtAuxPost
 * 5   StmtTail
 */
void FEFunctionDemo::LoadGenStmtDemo2() {
  Init();
  mapIdxStmt.clear();
  (void)NewGenStmt<GeneralStmtAuxPre>(1);
  (void)NewGenStmt<FEIRStmt>(2);
  FEIRStmt *stmt3 = NewGenStmt<FEIRStmt>(3);
  (void)NewGenStmt<GeneralStmtAuxPost>(4);
  stmt3->SetFallThru(false);
}

/* GenStmtDemo3:CFG
 *     --- BB0 ---
 * 0   StmtHead
 *     --- BB1 ---
 * 1   StmtAuxPre
 * 2   StmtMultiOut (fallthru = true, out = {8})
 * 3   StmtAuxPost
 *     --- BB2 ---
 * 4   StmtAuxPre
 * 5   Stmt (fallthru = false)
 * 6   StmtAuxPost
 *     --- BB3 ---
 * 7   StmtAuxPre
 * 8   StmtMultiIn (fallthru = true, in = {2})
 * 9   Stmt (fallthru = false)
 * 10  StmtAuxPos
 *     --- BB4 ---
 * 11  StmtTail
 *
 * GenStmtDemo3_CFG:
 *      BB0
 *       |
 *      BB1
 *      / \
 *   BB2   BB3
 */
void FEFunctionDemo::LoadGenStmtDemo3() {
  Init();
  mapIdxStmt.clear();
  // --- BB1 ---
  (void)NewGenStmt<GeneralStmtAuxPre>(1);
  FEIRStmt *stmt2 = NewGenStmt<FEIRStmt>(2);
  (void)NewGenStmt<GeneralStmtAuxPost>(3);
  // --- BB2 ---
  (void)NewGenStmt<GeneralStmtAuxPre>(4);
  FEIRStmt *stmt5 = NewGenStmt<FEIRStmt>(5);
  stmt5->SetFallThru(false);
  (void)NewGenStmt<GeneralStmtAuxPost>(6);
  // --- BB3 ---
  (void)NewGenStmt<GeneralStmtAuxPre>(7);
  FEIRStmt *stmt8 = NewGenStmt<FEIRStmt>(8);
  FEIRStmt *stmt9 = NewGenStmt<FEIRStmt>(9);
  stmt9->SetFallThru(false);
  (void)NewGenStmt<GeneralStmtAuxPost>(10);
  // Link
  stmt2->AddSucc(*stmt8);
  stmt8->AddPred(*stmt2);
}

/* GenStmtDemo4:CFG_Fail
 * 0   StmtHead
 * 1   Stmt (fallthru = true)
 * 2   Stmt (fallthru = true)
 * 3   StmtTail
 */
void FEFunctionDemo::LoadGenStmtDemo4() {
  Init();
  mapIdxStmt.clear();
  (void)NewGenStmt<FEIRStmt>(1);
  (void)NewGenStmt<FEIRStmt>(2);
}

/* GenStmtDemo5:CFG_DeadBB
 *     --- BB0 ---
 * 0   StmtHead
 *     --- BB1 ---
 * 1   Stmt (fallthru = true)
 * 2   Stmt (fallthru = false)
 *     --- BB2 ---
 * 3   Stmt (fallthru = false)
 *     --- BB3 ---
 * 4   StmtTail
 *
 * GenStmtDemo5_CFG:
 *      BB0
 *       |
 *      BB1  BB2(DeadBB)
 */
void FEFunctionDemo::LoadGenStmtDemo5() {
  Init();
  mapIdxStmt.clear();
  (void)NewGenStmt<FEIRStmt>(1);
  FEIRStmt *stmt2 = NewGenStmt<FEIRStmt>(2);
  stmt2->SetFallThru(false);
  FEIRStmt *stmt3 = NewGenStmt<FEIRStmt>(3);
  stmt3->SetFallThru(false);
}
}  // namespace maple
