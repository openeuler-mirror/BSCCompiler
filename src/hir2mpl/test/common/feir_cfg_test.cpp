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
#include "feir_cfg.h"
#include "redirect_buffer.h"
#include "hir2mpl_ut_environment.h"

namespace maple {
class GeneralStmtHead : public FEIRStmt {
 public:
  GeneralStmtHead() : FEIRStmt(kStmtPesudo) {}
  ~GeneralStmtHead() = default;
};

class GeneralStmtTail : public FEIRStmt {
 public:
  GeneralStmtTail() : FEIRStmt(kStmtPesudo) {}
  ~GeneralStmtTail() = default;
};

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

class FEIRCFGDemo : public FEIRCFG {
 public:
  FEIRCFGDemo(MapleAllocator &argAllocator, FEIRStmt *argStmtHead, FEIRStmt *argStmtTail)
      : FEIRCFG(argStmtHead, argStmtTail),
        allocator(argAllocator),
        mapIdxStmt(allocator.Adapter()) {}
  ~FEIRCFGDemo() = default;

  void LoadGenStmtDemo1();
  void LoadGenStmtDemo2();
  void LoadGenStmtDemo3();
  void LoadGenStmtDemo4();
  void LoadGenStmtDemo5();
  void LoadGenStmtDemo6();
  void LoadGenStmtDemo7();
  void LoadGenStmtDemo8();

  FEIRStmt *GetStmtByIdx(uint32 idx) {
    CHECK_FATAL(mapIdxStmt.find(idx) != mapIdxStmt.end(), "invalid idx");
    return mapIdxStmt[idx];
  }

  template <typename T>
  T *NewTemporaryStmt(uint32 idx) {
    FEIRStmt *ptrStmt = allocator.New<T>();
    ptrStmt->SetFallThru(true);
    stmtTail->InsertBefore(ptrStmt);
    mapIdxStmt[idx] = ptrStmt;
    return static_cast<T*>(ptrStmt);
  }

 private:
  MapleAllocator allocator;
  MapleMap<uint32, FEIRStmt*> mapIdxStmt;
};

class FEIRCFGTest : public testing::Test, public RedirectBuffer {
 public:
  static MemPool *mp;
  static FEIRStmt *genStmtHead;
  static FEIRStmt *genStmtTail;
  MapleAllocator allocator;
  FEIRCFGDemo demoCFG;
  FEIRCFGTest()
      : allocator(mp),
        demoCFG(allocator, genStmtHead, genStmtTail) {}
  ~FEIRCFGTest() = default;

  static void SetUpTestCase() {
    mp = FEUtils::NewMempool("MemPool for FEIRCFGTest", false /* isLocalPool */);
    genStmtHead = mp->New<GeneralStmtHead>();
    genStmtTail = mp->New<GeneralStmtTail>();
  }

  static void TearDownTestCase() {
    delete mp;
    mp = nullptr;
  }

  virtual void SetUp() {
    // reset head and tail stmt
    genStmtHead->SetNext(genStmtTail);
    genStmtTail->SetPrev(genStmtHead);
  }
};
MemPool *FEIRCFGTest::mp = nullptr;
FEIRStmt *FEIRCFGTest::genStmtHead = nullptr;
FEIRStmt *FEIRCFGTest::genStmtTail = nullptr;

/* GenStmtDemo1:BB
 * 0   StmtHead
 * 1   Stmt (fallthru = true)
 * 2   Stmt (fallthru = false)
 * 3   StmtTail
 */
void FEIRCFGDemo::LoadGenStmtDemo1() {
  mapIdxStmt.clear();
  (void)NewTemporaryStmt<FEIRStmt>(1);
  FEIRStmt *stmt2 = NewTemporaryStmt<FEIRStmt>(2);
  stmt2->SetFallThru(false);
}

TEST_F(FEIRCFGTest, CFGBuildForBB) {
  demoCFG.LoadGenStmtDemo1();
  demoCFG.Init();
  demoCFG.LabelStmtID();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelBBID();
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  EXPECT_EQ(bb1->GetNext(), demoCFG.GetDummyTail());
  EXPECT_EQ(bb1->GetStmtHead()->GetID(), 1);
  EXPECT_EQ(bb1->GetStmtTail()->GetID(), 2);
  EXPECT_EQ(demoCFG.HasDeadBB(), false);
}

/* GenStmtDemo2:BB_StmtAux
 * 0   StmtHead
 * 1   StmtAuxPre
 * 2   Stmt (fallthru = true)
 * 3   Stmt (fallthru = false)
 * 4   StmtAuxPost
 * 5   StmtTail
 */
void FEIRCFGDemo::LoadGenStmtDemo2() {
  mapIdxStmt.clear();
  (void)NewTemporaryStmt<GeneralStmtAuxPre>(1);
  (void)NewTemporaryStmt<FEIRStmt>(2);
  FEIRStmt *stmt3 = NewTemporaryStmt<FEIRStmt>(3);
  (void)NewTemporaryStmt<GeneralStmtAuxPost>(4);
  stmt3->SetFallThru(false);
}

TEST_F(FEIRCFGTest, CFGBuildForBB_StmtAux) {
  demoCFG.LoadGenStmtDemo2();
  demoCFG.Init();
  demoCFG.LabelStmtID();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelBBID();
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  EXPECT_EQ(bb1->GetNext(), demoCFG.GetDummyTail());
  EXPECT_EQ(bb1->GetStmtHead()->GetID(), 1);
  EXPECT_EQ(bb1->GetStmtTail()->GetID(), 4);
  EXPECT_EQ(bb1->GetStmtNoAuxHead()->GetID(), 2);
  EXPECT_EQ(bb1->GetStmtNoAuxTail()->GetID(), 3);
  EXPECT_EQ(demoCFG.HasDeadBB(), false);
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
void FEIRCFGDemo::LoadGenStmtDemo3() {
  mapIdxStmt.clear();
  // --- BB1 ---
  (void)NewTemporaryStmt<GeneralStmtAuxPre>(1);
  FEIRStmt *stmt2 = NewTemporaryStmt<FEIRStmt>(2);
  (void)NewTemporaryStmt<GeneralStmtAuxPost>(3);
  // --- BB2 ---
  (void)NewTemporaryStmt<GeneralStmtAuxPre>(4);
  FEIRStmt *stmt5 = NewTemporaryStmt<FEIRStmt>(5);
  stmt5->SetFallThru(false);
  (void)NewTemporaryStmt<GeneralStmtAuxPost>(6);
  // --- BB3 ---
  (void)NewTemporaryStmt<GeneralStmtAuxPre>(7);
  FEIRStmt *stmt8 = NewTemporaryStmt<FEIRStmt>(8);
  FEIRStmt *stmt9 = NewTemporaryStmt<FEIRStmt>(9);
  stmt9->SetFallThru(false);
  (void)NewTemporaryStmt<GeneralStmtAuxPost>(10);
  // Link
  stmt2->AddExtraSucc(*stmt8);
  stmt8->AddExtraPred(*stmt2);
}

TEST_F(FEIRCFGTest, CFGBuildForCFG) {
  demoCFG.LoadGenStmtDemo3();
  demoCFG.LabelStmtID();
  demoCFG.Init();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelBBID();
  // Check BB
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  ASSERT_NE(bb1, demoCFG.GetDummyTail());
  FEIRBB *bb2 = static_cast<FEIRBB*>(bb1->GetNext());
  ASSERT_NE(bb2, demoCFG.GetDummyTail());
  FEIRBB *bb3 = static_cast<FEIRBB*>(bb2->GetNext());
  ASSERT_NE(bb3, demoCFG.GetDummyTail());
  // Check BB's detail
  EXPECT_EQ(bb1->GetStmtHead()->GetID(), 1);
  EXPECT_EQ(bb1->GetStmtNoAuxHead()->GetID(), 2);
  EXPECT_EQ(bb1->GetStmtNoAuxTail()->GetID(), 2);
  EXPECT_EQ(bb1->GetStmtTail()->GetID(), 3);
  EXPECT_EQ(bb2->GetStmtHead()->GetID(), 4);
  EXPECT_EQ(bb2->GetStmtNoAuxHead()->GetID(), 5);
  EXPECT_EQ(bb2->GetStmtNoAuxTail()->GetID(), 5);
  EXPECT_EQ(bb2->GetStmtTail()->GetID(), 6);
  EXPECT_EQ(bb3->GetStmtHead()->GetID(), 7);
  EXPECT_EQ(bb3->GetStmtNoAuxHead()->GetID(), 8);
  EXPECT_EQ(bb3->GetStmtNoAuxTail()->GetID(), 9);
  EXPECT_EQ(bb3->GetStmtTail()->GetID(), 10);
  // Check CFG
  EXPECT_EQ(bb1->GetPredBBs().size(), 1);
  EXPECT_EQ(bb1->IsPredBB(0U), true);
  EXPECT_EQ(bb1->GetSuccBBs().size(), 2);
  EXPECT_EQ(bb1->IsSuccBB(2), true);
  EXPECT_EQ(bb1->IsSuccBB(3), true);
  EXPECT_EQ(bb2->GetPredBBs().size(), 1);
  EXPECT_EQ(bb2->IsPredBB(1), true);
  EXPECT_EQ(bb2->GetSuccBBs().size(), 0);
  EXPECT_EQ(bb3->GetPredBBs().size(), 1);
  EXPECT_EQ(bb3->IsPredBB(1), true);
  EXPECT_EQ(bb3->GetSuccBBs().size(), 0);
  EXPECT_EQ(demoCFG.HasDeadBB(), false);
}

/* GenStmtDemo4:CFG_Fail
 * 0   StmtHead
 * 1   Stmt (fallthru = true)
 * 2   Stmt (fallthru = true)
 * 3   StmtTail
 */
void FEIRCFGDemo::LoadGenStmtDemo4() {
  mapIdxStmt.clear();
  (void)NewTemporaryStmt<FEIRStmt>(1);
  (void)NewTemporaryStmt<FEIRStmt>(2);
}

TEST_F(FEIRCFGTest, CFGBuildForCFG_Fail) {
  demoCFG.Init();
  demoCFG.LoadGenStmtDemo4();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, false);
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
void FEIRCFGDemo::LoadGenStmtDemo5() {
  mapIdxStmt.clear();
  (void)NewTemporaryStmt<FEIRStmt>(1);
  FEIRStmt *stmt2 = NewTemporaryStmt<FEIRStmt>(2);
  stmt2->SetFallThru(false);
  FEIRStmt *stmt3 = NewTemporaryStmt<FEIRStmt>(3);
  stmt3->SetFallThru(false);
}

TEST_F(FEIRCFGTest, CFGBuildForCFG_DeadBB) {
  demoCFG.Init();
  demoCFG.LoadGenStmtDemo5();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelStmtID();
  demoCFG.LabelBBID();
  // Check BB
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  ASSERT_NE(bb1, demoCFG.GetDummyTail());
  FEIRBB *bb2 = static_cast<FEIRBB*>(bb1->GetNext());
  ASSERT_NE(bb2, demoCFG.GetDummyTail());
  // Check BB's detail
  EXPECT_EQ(bb1->GetStmtHead()->GetID(), 1);
  EXPECT_EQ(bb1->GetStmtTail()->GetID(), 2);
  EXPECT_EQ(bb2->GetStmtHead()->GetID(), 3);
  EXPECT_EQ(bb2->GetStmtTail()->GetID(), 3);
  // Check CFG
  EXPECT_EQ(bb1->GetPredBBs().size(), 1);
  EXPECT_EQ(bb1->IsPredBB(0U), true);
  EXPECT_EQ(bb2->GetSuccBBs().size(), 0);
  EXPECT_EQ(demoCFG.HasDeadBB(), true);
}

/* GenStmtDemo6:CFG
 *     --- BB0 ---
 * 0   StmtHead
 *     --- BB1 ---
 * 1   StmtMultiOut (fallthru = true, out = {3})
 *     --- BB2 ---
 * 2   Stmt (fallthru = true)
 *     --- BB3 ---
 * 3   StmtMultiIn (fallthru = true, in = {1})
 * 4   Stmt (fallthru = false)
 *     --- BB4 ---
 * 5   StmtTail
 *
 * GenStmtDemo6_CFG:
 *      BB0
 *       |
 *      BB1
 *      / \
 *   BB2   |
 *      \ /
 *      BB3
 */
void FEIRCFGDemo::LoadGenStmtDemo6() {
  mapIdxStmt.clear();
  // --- BB1 ---
  FEIRStmt *stmt1 = NewTemporaryStmt<FEIRStmt>(1);
  // --- BB2 ---
  (void)NewTemporaryStmt<FEIRStmt>(2);
  // --- BB3 ---
  FEIRStmt *stmt3 = NewTemporaryStmt<FEIRStmt>(3);
  FEIRStmt *stmt4 = NewTemporaryStmt<FEIRStmt>(4);
  stmt4->SetFallThru(false);
  // Link
  stmt1->AddExtraSucc(*stmt3);
  stmt3->AddExtraPred(*stmt1);
}

TEST_F(FEIRCFGTest, CFGBuildForCFG1) {
  demoCFG.Init();
  demoCFG.LoadGenStmtDemo6();
  demoCFG.LabelStmtID();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelBBID();
  // Check BB
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  ASSERT_NE(bb1, demoCFG.GetDummyTail());
  FEIRBB *bb2 = static_cast<FEIRBB*>(bb1->GetNext());
  ASSERT_NE(bb2, demoCFG.GetDummyTail());
  FEIRBB *bb3 = static_cast<FEIRBB*>(bb2->GetNext());
  ASSERT_NE(bb3, demoCFG.GetDummyTail());
  // Check CFG
  EXPECT_EQ(bb1->GetPredBBs().size(), 1);
  EXPECT_EQ(bb1->IsPredBB(0U), true);
  EXPECT_EQ(bb1->GetSuccBBs().size(), 2);
  EXPECT_EQ(bb1->IsSuccBB(2), true);
  EXPECT_EQ(bb1->IsSuccBB(3), true);
  EXPECT_EQ(bb2->GetPredBBs().size(), 1);
  EXPECT_EQ(bb2->IsPredBB(1), true);
  EXPECT_EQ(bb2->GetSuccBBs().size(), 1);
  EXPECT_EQ(bb2->IsSuccBB(3), true);
  EXPECT_EQ(bb3->GetPredBBs().size(), 2);
  EXPECT_EQ(bb3->IsPredBB(1), true);
  EXPECT_EQ(bb3->IsPredBB(2), true);
  EXPECT_EQ(demoCFG.HasDeadBB(), false);
}

/* GenStmtDemo7:CFG
 *     --- BB0 ---
 * 0   StmtHead
 *     --- BB1 ---
 * 1   StmtMultiOut (fallthru = true, out = {5})
 *     --- BB2 ---
 * 2   Stmt (fallthru = true)
 *     --- BB3 ---
 * 3   StmtMultiIn (fallthru = true, in = {6})
 * 4   Stmt (fallthru = false)
 *     --- BB4 ---
 * 5   StmtMultiIn (fallthru = true, in = {1})
 * 6   Stmt (fallthru = false, out = {3})
 *     --- BB5 ---
 * 7   StmtTail
 *
 * GenStmtDemo7_CFG:
 *      BB0
 *       |
 *      BB1
 *      / \
 *   BB2   BB4
 *      \ /
 *      BB3
 */
void FEIRCFGDemo::LoadGenStmtDemo7() {
  mapIdxStmt.clear();
  // --- BB1 ---
  FEIRStmt *stmt1 = NewTemporaryStmt<FEIRStmt>(1);
  // --- BB2 ---
  (void)NewTemporaryStmt<FEIRStmt>(2);
  // --- BB3 ---
  FEIRStmt *stmt3 = NewTemporaryStmt<FEIRStmt>(3);
  FEIRStmt *stmt4 = NewTemporaryStmt<FEIRStmt>(4);
  stmt4->SetFallThru(false);
  // --- BB4 ---
  FEIRStmt *stmt5 = NewTemporaryStmt<FEIRStmt>(5);
  FEIRStmt *stmt6 = NewTemporaryStmt<FEIRStmt>(6);
  stmt6->SetFallThru(false);
  // Link
  stmt1->AddExtraSucc(*stmt5);
  stmt5->AddExtraPred(*stmt1);
  stmt6->AddExtraSucc(*stmt3);
  stmt3->AddExtraPred(*stmt6);
}

TEST_F(FEIRCFGTest, CFGBuildForCFG2) {
  demoCFG.Init();
  demoCFG.LoadGenStmtDemo7();
  demoCFG.LabelStmtID();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelBBID();
  // Check BB
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  ASSERT_NE(bb1, demoCFG.GetDummyTail());
  FEIRBB *bb2 = static_cast<FEIRBB*>(bb1->GetNext());
  ASSERT_NE(bb2, demoCFG.GetDummyTail());
  FEIRBB *bb3 = static_cast<FEIRBB*>(bb2->GetNext());
  ASSERT_NE(bb3, demoCFG.GetDummyTail());
  FEIRBB *bb4 = static_cast<FEIRBB*>(bb3->GetNext());
  ASSERT_NE(bb4, demoCFG.GetDummyTail());
  // Check CFG
  EXPECT_EQ(bb2->GetPredBBs().size(), 1);
  EXPECT_EQ(bb2->IsPredBB(1), true);
  EXPECT_EQ(bb2->GetSuccBBs().size(), 1);
  EXPECT_EQ(bb2->IsSuccBB(3), true);
  EXPECT_EQ(bb4->GetPredBBs().size(), 1);
  EXPECT_EQ(bb4->IsPredBB(1), true);
  EXPECT_EQ(bb4->GetSuccBBs().size(), 1);
  EXPECT_EQ(bb4->IsSuccBB(3), true);
  EXPECT_EQ(bb3->GetPredBBs().size(), 2);
  EXPECT_EQ(bb3->IsPredBB(2), true);
  EXPECT_EQ(bb3->IsPredBB(4), true);
  EXPECT_EQ(demoCFG.HasDeadBB(), false);
}

/* GenStmtDemo8:CFG
 *     --- BB0 ---
 * 0   StmtHead
 *     --- BB1 ---
 * 1   StmtMultiOut (fallthru = true, out = {6})
 *     --- BB2 ---
 * 2   StmtMultiIn (fallthru = true, in = {5})
 * 3   Stmt (fallthru = true， out = {4})
 *     --- BB3 ---
 * 4   Stmt (fallthru = true, in = {3})
 * 5   StmtMultiOut (fallthru = true, in = {2})
 *     --- BB4 ---
 * 6   StmtMultiIn (fallthru = true, in = {1})
 * 7   Stmt (fallthru = false)
 *     --- BB5 ---
 * 8   StmtTail
 *
 * GenStmtDemo8_CFG_while:
 *      BB0
 *       |
 *      BB1 -----
 *       |       |
 *      BB2 <-   |
 *       |    |  |
 *      BB3 --   |
 *       |       |
 *      BB4 <----
 */
void FEIRCFGDemo::LoadGenStmtDemo8() {
  mapIdxStmt.clear();
  // --- BB1 ---
  FEIRStmt *stmt1 = NewTemporaryStmt<FEIRStmt>(1);
  // --- BB2 ---
  FEIRStmt *stmt2 = NewTemporaryStmt<FEIRStmt>(2);
  FEIRStmt *stmt3 = NewTemporaryStmt<FEIRStmt>(3);
  // --- BB3 ---
  FEIRStmt *stmt4 = NewTemporaryStmt<FEIRStmt>(4);
  FEIRStmt *stmt5 = NewTemporaryStmt<FEIRStmt>(5);
  // --- BB4 ---
  FEIRStmt *stmt6 = NewTemporaryStmt<FEIRStmt>(6);
  FEIRStmt *stmt7 = NewTemporaryStmt<FEIRStmt>(7);
  stmt7->SetFallThru(false);
  // Link
  stmt1->AddExtraSucc(*stmt6);
  stmt6->AddExtraPred(*stmt1);
  stmt5->AddExtraSucc(*stmt2);
  stmt2->AddExtraPred(*stmt5);
  stmt3->AddExtraSucc(*stmt4);
  stmt4->AddExtraPred(*stmt3);
}

TEST_F(FEIRCFGTest, CFGBuildForCFG_while) {
  demoCFG.Init();
  demoCFG.LoadGenStmtDemo8();
  demoCFG.LabelStmtID();
  demoCFG.BuildBB();
  bool resultCFG = demoCFG.BuildCFG();
  ASSERT_EQ(resultCFG, true);
  demoCFG.LabelBBID();
  // Check BB
  demoCFG.DumpBBs();
  FEIRBB *bb1 = static_cast<FEIRBB*>(demoCFG.GetDummyHead()->GetNext());
  ASSERT_NE(bb1, demoCFG.GetDummyTail());
  FEIRBB *bb2 = static_cast<FEIRBB*>(bb1->GetNext());
  ASSERT_NE(bb2, demoCFG.GetDummyTail());
  FEIRBB *bb3 = static_cast<FEIRBB*>(bb2->GetNext());
  ASSERT_NE(bb3, demoCFG.GetDummyTail());
  FEIRBB *bb4 = static_cast<FEIRBB*>(bb3->GetNext());
  ASSERT_NE(bb4, demoCFG.GetDummyTail());
  // Check CFG
  EXPECT_EQ(bb1->GetSuccBBs().size(), 2);
  EXPECT_EQ(bb1->IsSuccBB(2), true);
  EXPECT_EQ(bb1->IsSuccBB(4), true);
  EXPECT_EQ(bb2->GetPredBBs().size(), 2);
  EXPECT_EQ(bb2->IsPredBB(1), true);
  EXPECT_EQ(bb2->IsPredBB(3), true);
  EXPECT_EQ(bb2->GetSuccBBs().size(), 1);
  EXPECT_EQ(bb2->IsSuccBB(3), true);
  EXPECT_EQ(bb3->GetPredBBs().size(), 1);
  EXPECT_EQ(bb3->IsPredBB(2), true);
  EXPECT_EQ(bb3->GetSuccBBs().size(), 2);
  EXPECT_EQ(bb3->IsSuccBB(2), true);
  EXPECT_EQ(bb3->IsSuccBB(4), true);
  EXPECT_EQ(bb4->GetPredBBs().size(), 2);
  EXPECT_EQ(bb4->IsPredBB(1), true);
  EXPECT_EQ(bb4->IsPredBB(3), true);
  EXPECT_EQ(demoCFG.HasDeadBB(), false);
}
}  // namespace maple
