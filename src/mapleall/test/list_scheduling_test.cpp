/*
* Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "triple.h"
#include "gtest/gtest.h"
#include "mempool.h"
#include "mpl_logging.h"
#include "becommon.h"
#include "cg.h"
#include "cg_irbuilder.h"
#include "deps.h"
#include "cg_cdg.h"
#include "list_scheduler.h"
#include "data_dep_analysis.h"
#include "control_dep_analysis.h"
#include "aarch64_data_dep_base.h"
#include "aarch64_local_schedule.h"
#include "aarch64_cg.h"
#include "aarch64_cgfunc.h"

using namespace maplebe;
#define TARGAARCH64 1

class MockListScheduler {
public:
 MockListScheduler(MemPool *mp, MapleAllocator *alloc)
     : memPool(mp), alloc(alloc), skMp(mp->New<StackMemPool>(memPoolCtrler, "ut_stack_mp")),
       mirModule(mp->New<MIRModule>("ut_test_ls")), options(mp->New<CGOptions>()),
       nameVec(mp->New<std::vector<std::string>>()),
       patternMap(mp->New<std::unordered_map<std::string, std::vector<std::string>>>()),
       aarCG(mp->New<AArch64CG>(*mirModule, *options, *nameVec, *patternMap)),
       mirFunc(mirModule->GetMIRBuilder()->GetOrCreateFunction("ut_test_list_scheduling", TyIdx(PTY_void))),
       beCommon(mp->New<BECommon>(*mirModule)) {
   mirFunc->AllocSymTab();
   mirFunc->AllocPregTab();
   mirFunc->AllocLabelTab();

   Globals::GetInstance()->SetTarget(*aarCG);
   aarFunc = memPool->New<AArch64CGFunc>(*mirModule, *aarCG, *mirFunc, *beCommon, *mp, *skMp, *alloc, 0);

   mad = memPool->New<MAD>();
   ddb = memPool->New<AArch64DataDepBase>(*mp, *aarFunc, *mad, true);
   dda = memPool->New<DataDepAnalysis>(*aarFunc, *mp, *ddb);
   cda = memPool->New<ControlDepAnalysis>(*aarFunc, *mp, "ut_test_ls_cda");
   aarLS = memPool->New<AArch64LocalSchedule>(*mp, *aarFunc, *cda, *dda);
 }
 ~MockListScheduler() = default;

 AArch64CGFunc *GetAArch64CGFunc() {
   return aarFunc;
 }

 AArch64LocalSchedule *GetAArch64LocalSchedule() {
   return aarLS;
 }

 DataDepAnalysis *GetDataDepAnalysis() {
   return dda;
 }

 MAD *GetMAD() {
   return mad;
 }

 // Create CDGNode for mock BB
 CDGNode *CreateCDGNode(maplebe::BB &bb) {
   auto *cdgNode = memPool->New<CDGNode>(CDGNodeId(bb.GetId()), bb, *alloc);
   bb.SetCDGNode(cdgNode);
   auto *region = memPool->New<CDGRegion>(CDGRegionId(1), *alloc);
   region->AddCDGNode(cdgNode);
   cdgNode->SetRegion(*region);
   region->SetRegionRoot(*cdgNode);
   return cdgNode;
 }

 // Run local scheduler for mock BB
 void DoLocalScheduleForMockBB(CDGNode &cdgNode) {
   CDGRegion *region = cdgNode.GetRegion();
   ASSERT_NOT_NULL(region);
   aarLS->SetUnitTest(true);
   aarLS->DoLocalScheduleForRegion(*region);
 }

 // Run global scheduler for mock BB
 void DoGlobalScheduleForMockBB(CDGRegion &region) {
   aarGS = memPool->New<AArch64GlobalSchedule>(*memPool, *aarFunc, *cda, *dda);
   aarGS->Run();
 }

private:
 MemPool *memPool = nullptr;
 MapleAllocator *alloc = nullptr;
 StackMemPool *skMp = nullptr;
 MIRModule *mirModule = nullptr;
 CGOptions *options = nullptr;
 std::vector<std::string> *nameVec = nullptr;
 std::unordered_map<std::string, std::vector<std::string>> *patternMap = nullptr;
 AArch64CG *aarCG = nullptr;
 MIRFunction *mirFunc = nullptr;
 BECommon *beCommon = nullptr;
 AArch64CGFunc *aarFunc = nullptr;
 MAD *mad = nullptr;
 ControlDepAnalysis *cda = nullptr;
 AArch64DataDepBase *ddb = nullptr;
 DataDepAnalysis *dda = nullptr;
 AArch64LocalSchedule *aarLS = nullptr;
 AArch64GlobalSchedule *aarGS = nullptr;
};

/* 1.0 old version */
// Test process of list scheduling in local BB
TEST(listSchedule, testListSchedulingInLocalBB1) {
 Triple::GetTriple().Init();
 auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
 auto *alloc = new MapleAllocator(memPool);
 MockListScheduler mock(memPool, alloc);
 AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

 // Mock maple BB info using gcc BB info before gcc scheduling
 auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
 aarFunc->SetCurBB(*curBB);
 aarFunc->SetFirstBB(*curBB);

 // r314=[r313+0x1b]            (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
 MOperator mop1 = MOP_wldrb;
 auto R314 = static_cast<regno_t>(314);
 RegOperand *dstOpnd1 = aarFunc->CreateVirtualRegisterOperand(R314, maplebe::k32BitSize, kRegTyInt);
 auto R313 = static_cast<regno_t>(313);
 RegOperand *baseOpnd1 = aarFunc->CreateVirtualRegisterOperand(R313, maplebe::k64BitSize, kRegTyInt);
 maple::int64 offset1 = 27;
 MemOperand &memOpnd1 = aarFunc->CreateMemOpnd(*baseOpnd1, offset1, maplebe::k8BitSize);
 maplebe::Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(mop1, *dstOpnd1, memOpnd1);
 insn1.SetId(1);
 curBB->AppendInsn(insn1);

 // r320=r314#0<<0x1&0x3e       cortex_a53_slot_any
 MOperator mop2 = MOP_xubfizrri6i6;
 auto R320 = static_cast<regno_t>(320);
 RegOperand *dstOpnd2 = aarFunc->CreateVirtualRegisterOperand(R320, maplebe::k64BitSize, kRegTyInt);
 ImmOperand &lsbOpnd = aarFunc->CreateImmOperand(1, maplebe::k64BitSize, false);
 ImmOperand &widthOpnd = aarFunc->CreateImmOperand(5, maplebe::k64BitSize, false);
 maplebe::Insn &insn2 = aarFunc->GetInsnBuilder()->BuildInsn(mop2, *dstOpnd2, *dstOpnd1, lsbOpnd, widthOpnd);
 insn2.SetId(2);
 curBB->AppendInsn(insn2);

 // r322=r313+r320              cortex_a53_slot_any
 MOperator mop3 = MOP_xaddrrr;
 auto R322 = static_cast<regno_t>(322);
 RegOperand *dstOpnd3 = aarFunc->CreateVirtualRegisterOperand(R322, maplebe::k64BitSize, kRegTyInt);
 maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, *dstOpnd3, *baseOpnd1, *dstOpnd2);
 insn3.SetId(3);
 curBB->AppendInsn(insn3);

 // [r322+0x34]=r356            (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
 MOperator mop4 = MOP_wstrh;
 auto R356 = static_cast<regno_t>(356);
 RegOperand *srcOpnd4 = aarFunc->CreateVirtualRegisterOperand(R356, maplebe::k32BitSize, kRegTyInt);
 maple::int64 offset2 = 52;
 MemOperand &memOpnd4 = aarFunc->CreateMemOpnd(*dstOpnd3, offset2, maplebe::k16BitSize);
 maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, *srcOpnd4, memOpnd4);
 insn4.SetId(4);
 curBB->AppendInsn(insn4);

 // r327=r314#0+0x1             cortex_a53_slot_any
 MOperator mop5 = MOP_waddrri12;
 auto R327 = static_cast<regno_t>(327);
 RegOperand *dstOpnd5 = aarFunc->CreateVirtualRegisterOperand(R327, maplebe::k32BitSize, kRegTyInt);
 ImmOperand &immOpnd5 = aarFunc->CreateImmOperand(1, maplebe::k32BitSize, false);
 maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, *dstOpnd5, *dstOpnd1, immOpnd5);
 insn5.SetId(5);
 curBB->AppendInsn(insn5);

 // r329=r327&0x1f              cortex_a53_slot_any
 MOperator mop6 = MOP_wandrri12;
 auto R329 = static_cast<regno_t>(329);
 RegOperand *dstOpnd6 = aarFunc->CreateVirtualRegisterOperand(R329, maplebe::k32BitSize, kRegTyInt);
 ImmOperand &immOpnd6 = aarFunc->CreateImmOperand(31, maplebe::k32BitSize, false);
 maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, *dstOpnd6, *dstOpnd5, immOpnd6);
 insn6.SetId(6);
 curBB->AppendInsn(insn6);

 // [r313+0x1b]=r329#0          (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
 MOperator mop7 = MOP_wstrb;
 maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, *dstOpnd6, memOpnd1);
 insn7.SetId(7);
 curBB->AppendInsn(insn7);

 // pc=L291                     (cortex_a53_slot_any+cortex_a53_branch)
 MOperator mop8 = MOP_xuncond;
 std::string label = "ut_test_ls";
 auto *target = memPool->New<LabelOperand>(label.c_str(), 1, *memPool);
 maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildInsn(mop8, *target);
 insn8.SetId(8);
 curBB->AppendInsn(insn8);

 // Set max VRegNO
 aarFunc->SetMaxVReg(380);

 // Prepare data for list scheduling
 CDGNode *cdgNode = mock.CreateCDGNode(*curBB);

 // Execute local schedule using list scheduling algorithm on mock BB
 mock.DoLocalScheduleForMockBB(*cdgNode);

 delete memPool;
 delete alloc;
}

TEST(listSchedule, testListSchedulingInLocalBB2) {
  Triple::GetTriple().Init();
  auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
  auto *alloc = new MapleAllocator(memPool);
  MockListScheduler mock(memPool, alloc);
  AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

  // Mock maple BB info using gcc BB info before gcc scheduling
  auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
  aarFunc->SetCurBB(*curBB);
  aarFunc->SetFirstBB(*curBB);

  // {sp=sp-0x90;[sp-0x90]=x29;[sp-0x88]=x30;}       (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop1 = MOP_xstp;
  RegOperand &r29 = aarFunc->GetOrCreatePhysicalRegisterOperand(R29, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r30 = aarFunc->GetOrCreatePhysicalRegisterOperand(R30, maplebe::k64BitSize, kRegTyInt);
  RegOperand &sp = aarFunc->GetOrCreatePhysicalRegisterOperand(RSP, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset1 = -144;
  MemOperand &memOpnd1 = aarFunc->CreateMemOpnd(sp, offset1, maplebe::k64BitSize);
  memOpnd1.SetAddrMode(maplebe::MemOperand::AArch64AddressingMode::kPreIndex);
  maplebe::Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(mop1, r29, r30, memOpnd1);
  insn1.SetId(867);
  curBB->AppendInsn(insn1);

  // x29=sp+0                       cortex_a53_slot_any
  MOperator mop2 = MOP_xaddrri12;
  ImmOperand &imm2 = aarFunc->CreateImmOperand(0, maplebe::k64BitSize, false);
  maplebe::Insn &insn2 = aarFunc->GetInsnBuilder()->BuildInsn(mop2, r29, sp, imm2);
  insn2.SetId(868);
  curBB->AppendInsn(insn2);

  // [scratch]=unspec[sp,x29] 64    nothing
  // barrier

  // {[sp+0x10]=x19;[sp+0x18]=x20;}         (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop3 = MOP_xstp;
  RegOperand &r19 = aarFunc->GetOrCreatePhysicalRegisterOperand(R19, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r20 = aarFunc->GetOrCreatePhysicalRegisterOperand(R20, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset3 = 16;
  MemOperand &memOpnd3 = aarFunc->CreateMemOpnd(sp, offset3, maplebe::k64BitSize);
  maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, r19, r20, memOpnd3);
  insn3.SetId(870);
  curBB->AppendInsn(insn3);

  // {[sp+0x20]=x21;[sp+0x28]=x22;}         (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop4 = MOP_xstp;
  RegOperand &r21 = aarFunc->GetOrCreatePhysicalRegisterOperand(R21, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r22 = aarFunc->GetOrCreatePhysicalRegisterOperand(R22, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset4 = 32;
  MemOperand &memOpnd4 = aarFunc->CreateMemOpnd(sp, offset4, maplebe::k64BitSize);
  maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, r21, r22, memOpnd4);
  insn4.SetId(871);
  curBB->AppendInsn(insn4);

  // {[sp+0x30]=x23;[sp+0x38]=x24;}         (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop5 = MOP_xstp;
  RegOperand &r23 = aarFunc->GetOrCreatePhysicalRegisterOperand(R23, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r24 = aarFunc->GetOrCreatePhysicalRegisterOperand(R24, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset5 = 48;
  MemOperand &memOpnd5 = aarFunc->CreateMemOpnd(sp, offset5, maplebe::k64BitSize);
  maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, r23, r24, memOpnd5);
  insn5.SetId(872);
  curBB->AppendInsn(insn5);

  // {[sp+0x40]=x25;[sp+0x48]=x26;}         (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop6 = MOP_xstp;
  RegOperand &r25 = aarFunc->GetOrCreatePhysicalRegisterOperand(R25, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r26 = aarFunc->GetOrCreatePhysicalRegisterOperand(R26, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset6 = 64;
  MemOperand &memOpnd6 = aarFunc->CreateMemOpnd(sp, offset6, maplebe::k64BitSize);
  maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, r25, r26, memOpnd6);
  insn6.SetId(873);
  curBB->AppendInsn(insn6);

  // x23=x0                  cortex_a53_slot_any
  MOperator mop7 = MOP_xmovrr;
  RegOperand &r0 = aarFunc->GetOrCreatePhysicalRegisterOperand(R0, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, r23, r0);
  insn7.SetId(72);
  curBB->AppendInsn(insn7);

  // v0=const_vector         cortex_a53_slot0,cortex_a53_fp_alu_q
  MOperator mop8 = MOP_vmovvi;
  RegOperand &v0 = aarFunc->GetOrCreatePhysicalRegisterOperand(V0, maplebe::k128BitSize, RegType::kRegTyFloat);
  auto *vecSpec = memPool->New<VectorRegSpec>(maple::PTY_v4i32);
  ImmOperand &imm8 = aarFunc->CreateImmOperand(0, maplebe::k32BitSize, false);
  maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildVectorInsn(mop8, AArch64CG::kMd[mop8]);
  (void)insn8.AddOpndChain(v0).AddOpndChain(imm8);
  (void)insn8.PushRegSpecEntry(vecSpec);
  insn8.SetId(86);
  curBB->AppendInsn(insn8);

  // x20=x1-0x1                     cortex_a53_slot_any
  MOperator mop9 = MOP_wsubrri12;
  RegOperand &r1 = aarFunc->GetOrCreatePhysicalRegisterOperand(R1, maplebe::k64BitSize, kRegTyInt);
  ImmOperand &imm9 = aarFunc->CreateImmOperand(1, maplebe::k32BitSize, false);
  maplebe::Insn &insn9 = aarFunc->GetInsnBuilder()->BuildInsn(mop9, r20, r1, imm9);
  insn9.SetId(73);
  curBB->AppendInsn(insn9);

  // x0=x0+0x8                      cortex_a53_slot_any
  MOperator mop10 = MOP_xaddrri12;
  ImmOperand &imm10 = aarFunc->CreateImmOperand(8, maplebe::k32BitSize, false);
  maplebe::Insn &insn10 = aarFunc->GetInsnBuilder()->BuildInsn(mop10, r0, r0, imm10);
  insn10.SetId(74);
  curBB->AppendInsn(insn10);

  // x1=`g_mbufGlobalCtl'         (cortex_a53_single_issue+cortex_a53_ls_agen),(cortex_a53_load+cortex_a53_slot0),cortex_a53_load
  MOperator mop11 = MOP_xadrp;
  MIRSymbol globalCtl(1, kScopeGlobal);
  globalCtl.SetStorageClass(kScGlobal);
  globalCtl.SetSKind(kStVar);
  std::string strGlobalCtl("g_mbufGlobalCtl");
  globalCtl.SetNameStrIdx(strGlobalCtl);
  StImmOperand &stImmOpnd11 = aarFunc->CreateStImmOperand(globalCtl, 0, 0);
  maplebe::Insn &insn11 = aarFunc->GetInsnBuilder()->BuildInsn(mop11, r1, stImmOpnd11);
  insn11.SetId(745);
  curBB->AppendInsn(insn11);

  // x22=unspec[[x1+low(`g_mbufGlobalCtl')]] 24 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop12 = MOP_xldr;
  OfstOperand &ofstOpnd = aarFunc->CreateOfstOpnd(*stImmOpnd11.GetSymbol(), stImmOpnd11.GetOffset(), stImmOpnd11.GetRelocs());
  MemOperand *memOpnd12 = aarFunc->CreateMemOperand(maplebe::k64BitSize, r1, ofstOpnd, *stImmOpnd11.GetSymbol());
  maplebe::Insn &insn12 = aarFunc->GetInsnBuilder()->BuildInsn(mop12, r22, *memOpnd12);
  insn12.SetId(553);
  curBB->AppendInsn(insn12);

  // x20=zxn(x20)<<0x3+x0           cortex_a53_slot_any
  MOperator mop13 = MOP_xxwaddrrre;
  ExtendShiftOperand &uxtwOpnd = aarFunc->CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, 3, maplebe::k3BitSize);
  maplebe::Insn &insn13 = aarFunc->GetInsnBuilder()->BuildInsn(mop13, r20, r0, r20, uxtwOpnd);
  insn13.SetId(76);
  curBB->AppendInsn(insn13);

  // x21=`g_hpfHlogLevel'          (cortex_a53_single_issue+cortex_a53_ls_agen),(cortex_a53_load+cortex_a53_slot0),cortex_a53_load
  MOperator mop14 = MOP_xadrp;
  MIRSymbol hlogLevel(0, kScopeGlobal);
  hlogLevel.SetStorageClass(kScGlobal);
  hlogLevel.SetSKind(kStVar);
  std::string strHlog("g_hpfHlogLevel");
  hlogLevel.SetNameStrIdx(strHlog);
  StImmOperand &stImmOpnd14 = aarFunc->CreateStImmOperand(hlogLevel, 0, 0);
  maplebe::Insn &insn14 = aarFunc->GetInsnBuilder()->BuildInsn(mop14, r21, stImmOpnd14);
  insn14.SetId(645);
  curBB->AppendInsn(insn14);

  // x19=0x280                     cortex_a53_slot_any
  MOperator mop15 = MOP_wmovri32;
  ImmOperand &imm15 = aarFunc->CreateImmOperand(640, maplebe::k32BitSize, false);
  maplebe::Insn &insn15 = aarFunc->GetInsnBuilder()->BuildInsn(mop15, r19, imm15);
  insn15.SetId(506);
  curBB->AppendInsn(insn15);

  // [sp+0x50]=x27                  (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop16 = MOP_xstr;
  RegOperand &r27 = aarFunc->GetOrCreatePhysicalRegisterOperand(R27, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset16 = 80;
  MemOperand &memOpnd16 = aarFunc->CreateMemOpnd(sp, offset16, maplebe::k64BitSize);
  maplebe::Insn &insn16 = aarFunc->GetInsnBuilder()->BuildInsn(mop16, r27, memOpnd16);
  insn16.SetId(874);
  curBB->AppendInsn(insn16);

  // After RA:
  aarFunc->SetIsAfterRegAlloc();
  // Before RA:
  //aarFunc->SetMaxVReg(50);

  // Prepare data for list scheduling
  CDGNode *cdgNode = mock.CreateCDGNode(*curBB);

  // (1) Data dependency graph construction and execution scheduling are separated,
  //     in which each module can be modified:
  DataDepAnalysis *dda = mock.GetDataDepAnalysis();
  dda->Run(*cdgNode->GetRegion());
  // Mock consistent data dependency after running data-dep-analysis (turn on when in use)
  for (auto *depNode : cdgNode->GetAllDataNodes()) {
    Insn *insn = depNode->GetInsn();
    if (insn->GetId() == 870) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 871 || succInsn->GetId() == 872 || succInsn->GetId() == 873 ||
            succInsn->GetId() == 874 || succInsn->GetId() == 553) { // these edges need to be removed
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 871) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 872 || succInsn->GetId() == 873 || succInsn->GetId() == 874) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 872) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 553 || succInsn->GetId() == 873 || succInsn->GetId() == 874) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 873) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 553 || succInsn->GetId() == 874) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 553) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 874) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 745) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 874) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 645) {
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 874) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    }
  }

  // luid of insn in gcc, same as insnId of maple
//  insn3.SetId(6);
//  insn4.SetId(7);
//  insn5.SetId(8);
//  insn6.SetId(9);
//  insn7.SetId(10);
//  insn8.SetId(11);
//  insn9.SetId(12);
//  insn10.SetId(13);
//  insn11.SetId(14);
//  insn12.SetId(15);
//  insn13.SetId(16);
//  insn14.SetId(17);
//  insn15.SetId(18);
//  insn16.SetId(19);

  // Init in region
  AArch64LocalSchedule *aarLS = mock.GetAArch64LocalSchedule();
  aarLS->InitInRegion(*cdgNode->GetRegion());
  // Do list scheduling
  aarLS->SetUnitTest(true);
  aarLS->DoLocalSchedule(*cdgNode);

  // (2) Encapsulation method, which the data dependency graph cannot be modified:
  // Execute local schedule using list scheduling algorithm on mock BB
  //mock.DoLocalScheduleForMockBB(*cdgNode);

  delete memPool;
  delete alloc;
}

TEST(listSchedule, testListSchedulingInLocalBB3) {
  Triple::GetTriple().Init();
  auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
  auto *alloc = new MapleAllocator(memPool);
  MockListScheduler mock(memPool, alloc);
  AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

  // Mock maple BB info using gcc BB info before gcc scheduling
  auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
  aarFunc->SetCurBB(*curBB);
  aarFunc->SetFirstBB(*curBB);

  // {[sp+0x10]=x19;[sp+0x18]=x20;} (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop3 = MOP_xstp;
  RegOperand &sp = aarFunc->GetOrCreatePhysicalRegisterOperand(RSP, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r19 = aarFunc->GetOrCreatePhysicalRegisterOperand(R19, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r20 = aarFunc->GetOrCreatePhysicalRegisterOperand(R20, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset3 = 16;
  MemOperand &memOpnd3 = aarFunc->CreateMemOpnd(sp, offset3, maplebe::k64BitSize);
  maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, r19, r20, memOpnd3);
  insn3.SetId(1428);
  curBB->AppendInsn(insn3);

  // {[sp+0x20]=x21;[sp+0x28]=x22;} (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop4 = MOP_xstp;
  RegOperand &r21 = aarFunc->GetOrCreatePhysicalRegisterOperand(R21, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r22 = aarFunc->GetOrCreatePhysicalRegisterOperand(R22, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset4 = 32;
  MemOperand &memOpnd4 = aarFunc->CreateMemOpnd(sp, offset4, maplebe::k64BitSize);
  maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, r21, r22, memOpnd4);
  insn4.SetId(1429);
  curBB->AppendInsn(insn4);

  // {[sp+0x30]=x23;[sp+0x38]=x24;}         (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop5 = MOP_xstp;
  RegOperand &r23 = aarFunc->GetOrCreatePhysicalRegisterOperand(R23, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r24 = aarFunc->GetOrCreatePhysicalRegisterOperand(R24, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset5 = 48;
  MemOperand &memOpnd5 = aarFunc->CreateMemOpnd(sp, offset5, maplebe::k64BitSize);
  maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, r23, r24, memOpnd5);
  insn5.SetId(1430);
  curBB->AppendInsn(insn5);

  // x22=unspec[0] 57    (mrs)              cortex_a53_slot_any
  MOperator mop6 = MOP_mrs;
  auto &tpidr = aarFunc->CreateCommentOperand("tpidr_el0");
  maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, r22, tpidr);
  insn6.SetId(31);
  curBB->AppendInsn(insn6);

  // x21=high(`__stack_chk_guard')          cortex_a53_slot_any
  MOperator mop7 = MOP_xadrp;
  MIRSymbol scg(0, kScopeGlobal);
  scg.SetStorageClass(kScGlobal);
  scg.SetSKind(kStVar);
  std::string strSCG("__stack_chk_guard");
  scg.SetNameStrIdx(strSCG);
  StImmOperand &stImmOpnd7 = aarFunc->CreateStImmOperand(scg, 0, 0);
  maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, r21, stImmOpnd7);
  insn7.SetId(4);
  curBB->AppendInsn(insn7);

  // x20=x0                         cortex_a53_slot_any
  MOperator mop8 = MOP_xmovrr;
  RegOperand &r0 = aarFunc->GetOrCreatePhysicalRegisterOperand(R0, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildInsn(mop8, r20, r0);
  insn8.SetId(2);
  curBB->AppendInsn(insn8);

  // {x0=unspec[`g_fwd_thread_index'] 58;clobber x30;clobber cc;clobber x1;} (cortex_a53_slot_any+cortex_a53_branch)
  MOperator mop9 = MOP_tls_desc_call;
  MIRSymbol tls(0, kScopeGlobal);
  tls.SetStorageClass(kScGlobal);
  tls.SetSKind(kStVar);
  std::string srtTLS("tlsdesc:g_fwd_thread_index");
  tls.SetNameStrIdx(srtTLS);
  StImmOperand &stImmOpnd9 = aarFunc->CreateStImmOperand(tls, 0, 0);
  RegOperand &r1 = aarFunc->GetOrCreatePhysicalRegisterOperand(R1, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn9 = aarFunc->GetInsnBuilder()->BuildInsn(mop9, r0, r1, stImmOpnd9);
  insn9.SetId(30);
  curBB->AppendInsn(insn9);

  // x2=unspec[[x21+low(`__stack_chk_guard')]] 24 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop10 = MOP_xldr;
  RegOperand &r2 = aarFunc->GetOrCreatePhysicalRegisterOperand(R2, maplebe::k64BitSize, kRegTyInt);
  OfstOperand &ofstOpnd = aarFunc->CreateOfstOpnd(*stImmOpnd7.GetSymbol(), stImmOpnd7.GetOffset(), stImmOpnd7.GetRelocs());
  MemOperand *memOpnd10 = aarFunc->CreateMemOperand(maplebe::k64BitSize, r21, ofstOpnd, *stImmOpnd7.GetSymbol());
  maplebe::Insn &insn10 = aarFunc->GetInsnBuilder()->BuildInsn(mop10, r2, *memOpnd10);
  insn10.SetId(5);
  curBB->AppendInsn(insn10);

  // x4=x22+x0                      cortex_a53_slot_any
  MOperator mop11 = MOP_xaddrrr;
  RegOperand &r4 = aarFunc->GetOrCreatePhysicalRegisterOperand(R4, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn11 = aarFunc->GetInsnBuilder()->BuildInsn(mop11, r4, r22, r0);
  insn11.SetId(32);
  curBB->AppendInsn(insn11);

  // [sp+0x40]=x25                  (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop12 = MOP_xstr;
  RegOperand &r25 = aarFunc->GetOrCreatePhysicalRegisterOperand(R25, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset12 = 64;
  MemOperand &memOpnd12 = aarFunc->CreateMemOpnd(sp, offset12, maplebe::k64BitSize);
  maplebe::Insn &insn12 = aarFunc->GetInsnBuilder()->BuildInsn(mop12, r25, memOpnd12);
  insn12.SetId(1431);
  curBB->AppendInsn(insn12);

  // x1=zxn([x22+x0])               (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop13 = MOP_wldrb;
  MemOperand &memOpnd13 = aarFunc->CreateMemOpnd(r22, 0, maplebe::k8BitSize);
  memOpnd13.SetIndexRegister(r0);
  maplebe::Insn &insn13 = aarFunc->GetInsnBuilder()->BuildInsn(mop13, r1, memOpnd13);
  insn13.SetId(33);
  curBB->AppendInsn(insn13);

  // {[x29+0xd8]=unspec[[x2]] 66;x0=0;} cortex_a53_slot_any
  // Turn on when in use
  /*
   * DEFINE_MOP(MOP_ut_sps, {&OpndDesc::Reg64ID, &OpndDesc::Reg64IS, &OpndDesc::Reg64IS, &OpndDesc::Imm64, &OpndDesc::Imm64}, 0, kLtAlu, "stack_protect_set_di", "0,1,2,3,4", 1);
   * MOperator mop14 = MOP_ut_sps;  // pseudo insn for unit test
   * RegOperand &r29 = aarFunc->GetOrCreatePhysicalRegisterOperand(R29, maplebe::k64BitSize, kRegTyInt);
   * ImmOperand &immOpnd1 = aarFunc->CreateImmOperand(216, maplebe::k64BitSize, false);
   * ImmOperand &immOpnd2 = aarFunc->CreateImmOperand(0, maplebe::k64BitSize, false);
   * maplebe::Insn &insn14 = aarFunc->GetInsnBuilder()->BuildInsn(mop14, r0, r2, r29, immOpnd1, immOpnd2);
   * insn14.SetId(6);
   * curBB->AppendInsn(insn14);
   */

  MOperator mop15 = MOP_xstr;
  //RegOperand &r29 = aarFunc->GetOrCreatePhysicalRegisterOperand(R29, maplebe::k64BitSize, kRegTyInt);
  int offset15 = 216;
  MemOperand &memOpnd15 = aarFunc->CreateMemOpnd(sp, offset15, maplebe::k64BitSize);
  maplebe::Insn &insn15 = aarFunc->GetInsnBuilder()->BuildInsn(mop15, r0, memOpnd15);
  insn15.SetId(6);
  curBB->AppendInsn(insn15);

  // pc={(x1!=0)?L69:pc}            (cortex_a53_slot_any+cortex_a53_branch)
  MOperator mop16 = MOP_wcbnz;
  auto &labelOpnd = aarFunc->GetOrCreateLabelOperand(1);
  maplebe::Insn &insn16 = aarFunc->GetInsnBuilder()->BuildInsn(mop16, r1, labelOpnd);
  insn16.SetId(35);
  curBB->AppendInsn(insn16);

  //curBB->Dump();
  aarFunc->SetIsAfterRegAlloc();

  // Create CDGNode
  CDGNode *cdgNode = mock.CreateCDGNode(*curBB);
  // Build dependency
  DataDepAnalysis *dda = mock.GetDataDepAnalysis();
  dda->Run(*cdgNode->GetRegion());

  // mock data dependency for tls (turn on when in use)
  DepNode *depNode5 = nullptr;
  DepNode *depNode6 = nullptr;
  DepNode *depNode32 = nullptr;
  for (auto *depNode : cdgNode->GetAllDataNodes()) {
    Insn *insn = depNode->GetInsn();
    if (insn->GetId() == 6) {
      for (auto *succLink : depNode->GetSuccs()) {
        DepNode &succNode = succLink->GetTo();
        if (succNode.GetInsn()->GetId() == 35) {
          succLink->SetLatency(3);
          succLink->SetDepType(maplebe::kDependenceTypeOutput);
        }
      }
    } else if (insn->GetId() == 5) {
      for (auto *succLink : depNode->GetSuccs()) {
        DepNode &succNode = succLink->GetTo();
        if (succNode.GetInsn()->GetId() == 6) {
          succLink->SetLatency(2);
        }
      }
    }
    if (insn->GetId() == 6) {
      depNode6 = depNode;
    } else if (insn->GetId() == 35) {
      depNode32 = depNode;
    } else if (insn->GetId() == 5) {
      depNode5 = depNode;
    }
    if (insn->GetId() != 2 && insn->GetId() != 30) {
      // delete edge to tls
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 30) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 30) {
      // delete edge from tls to unrelated insn
      auto sucLinkIt = depNode->GetSuccsBegin();
      while (sucLinkIt != depNode->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 5 || succInsn->GetId() == 1431) {
          sucLinkIt = depNode->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    }
  }
  auto *depLink1 = memPool->New<DepLink>(*depNode32, *depNode6, kDependenceTypeAnti);
  depNode32->AddSucc(*depLink1);
  depNode6->AddPred(*depLink1);
  auto *depLink2 = memPool->New<DepLink>(*depNode5, *depNode6, kDependenceTypeTrue);
  depLink2->SetLatency(mock.GetMAD()->GetLatency(*depNode5->GetInsn(), *depNode6->GetInsn()));
  depNode5->AddSucc(*depLink2);
  depNode6->AddPred(*depLink2);

  // Init in region
  AArch64LocalSchedule *aarLS = mock.GetAArch64LocalSchedule();
  aarLS->InitInRegion(*cdgNode->GetRegion());
  // Do list scheduling
  aarLS->SetUnitTest(true);
  aarLS->DoLocalSchedule(*cdgNode);

  delete memPool;
  delete alloc;
}

/* 1.0 new version */
// HpfRecvDirectOther block5 after RA
TEST(listSchedule, testListSchedulingInLocalBlock5Sched2) {
  Triple::GetTriple().Init();
  auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
  auto *alloc = new MapleAllocator(memPool);
  MockListScheduler mock(memPool, alloc);
  AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

  // Mock maple BB info using gcc BB info before gcc scheduling
  auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
  aarFunc->SetCurBB(*curBB);
  aarFunc->SetFirstBB(*curBB);

  // x19=[x20]                      (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop1 = MOP_xldr;
  RegOperand &r19 = aarFunc->GetOrCreatePhysicalRegisterOperand(R19, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r20 = aarFunc->GetOrCreatePhysicalRegisterOperand(R20, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset1 = 0;
  MemOperand &memOpnd1 = aarFunc->CreateMemOpnd(r20, offset1, maplebe::k64BitSize);
  maplebe::Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(mop1, r19, memOpnd1);
  insn1.SetId(61);
  curBB->AppendInsn(insn1);

  // x1=x19                         cortex_a53_slot_any
  MOperator mop2 = MOP_xmovrr;
  RegOperand &r1 = aarFunc->GetOrCreatePhysicalRegisterOperand(R1, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn2 = aarFunc->GetInsnBuilder()->BuildInsn(mop2, r1, r19);
  insn2.SetId(330);
  curBB->AppendInsn(insn2);

  // x0=zxn([x19+0x16])             (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop3 = MOP_wldrh;
  RegOperand &r0 = aarFunc->GetOrCreatePhysicalRegisterOperand(R0, maplebe::k32BitSize, kRegTyInt);
  maple::int64 offset3 = 22;
  MemOperand &memOpnd3 = aarFunc->CreateMemOpnd(r19, offset3, maplebe::k64BitSize);
  maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, r0, memOpnd3);
  insn3.SetId(62);
  curBB->AppendInsn(insn3);

  //  x0=x0+0xc0                     cortex_a53_slot_any
  MOperator mop4 = MOP_xaddrri12;
  ImmOperand &immOpnd4 = aarFunc->CreateImmOperand(192, maplebe::k64BitSize, false);
  maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, r0, r0, immOpnd4);
  insn4.SetId(63);
  curBB->AppendInsn(insn4);

  // x0=x19+x0                      cortex_a53_slot_any
  MOperator mop5 = MOP_xaddrrr;
  maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, r0, r19, r0);
  insn5.SetId(64);
  curBB->AppendInsn(insn5);

  // x2=[x19+0x40]                  (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop6 = MOP_xldr;
  RegOperand &r2 = aarFunc->GetOrCreatePhysicalRegisterOperand(R2, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset6 = 64;
  MemOperand &memOpnd6 = aarFunc->CreateMemOpnd(r19, offset6, maplebe::k64BitSize);
  maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, r2, memOpnd6);
  insn6.SetId(65);
  curBB->AppendInsn(insn6);

  // cc=cmp(x2,x0)                  cortex_a53_slot_any
  MOperator mop7 = MOP_xcmprr;
  Operand &ccReg = aarFunc->GetOrCreateRflag();
  maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, ccReg, r2, r0);
  insn7.SetId(66);
  curBB->AppendInsn(insn7);

  // pc={(cc!=0)?L189:pc}           (cortex_a53_slot_any+cortex_a53_branch)
  MOperator mop8 = MOP_bne;
  auto &labelOpnd = aarFunc->GetOrCreateLabelOperand(1);
  maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildInsn(mop8, ccReg, labelOpnd);
  insn8.SetId(67);
  curBB->AppendInsn(insn8);

  aarFunc->SetIsAfterRegAlloc();

  // Prepare data for list scheduling
  CDGNode *cdgNode = mock.CreateCDGNode(*curBB);

  // Execute local schedule using list scheduling algorithm on mock BB
  mock.DoLocalScheduleForMockBB(*cdgNode);

  delete memPool;
  delete alloc;
}

// HpfRecvDirectOther block4 after RA
TEST(listSchedule, testListSchedulingInLocalBlock4Sched2) {
  Triple::GetTriple().Init();
  auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
  auto *alloc = new MapleAllocator(memPool);
  MockListScheduler mock(memPool, alloc);
  AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

  // Mock maple BB info using gcc BB info before gcc scheduling
  auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
  aarFunc->SetCurBB(*curBB);
  aarFunc->SetFirstBB(*curBB);

  // {asm_operands;clobber [scratch];} nothing
  MOperator mop1 = MOP_asm;
  Operand &asmString1 = aarFunc->CreateStringOperand("");
  ListOperand *listInputOpnd1 = aarFunc->CreateListOpnd(*alloc);
  ListOperand *listOutputOpnd1 = aarFunc->CreateListOpnd(*alloc);
  ListOperand *listClobber1 = aarFunc->CreateListOpnd(*alloc);
  auto *listInConstraint1 = memPool->New<ListConstraintOperand>(*alloc);
  auto *listOutConstraint1 = memPool->New<ListConstraintOperand>(*alloc);
  auto *listInRegPrefix1 = memPool->New<ListConstraintOperand>(*alloc);
  auto *listOutRegPrefix1 = memPool->New<ListConstraintOperand>(*alloc);
  std::vector<Operand*> intrnOpnds1;
  intrnOpnds1.emplace_back(&asmString1);
  intrnOpnds1.emplace_back(listOutputOpnd1);
  intrnOpnds1.emplace_back(listClobber1);
  intrnOpnds1.emplace_back(listInputOpnd1);
  intrnOpnds1.emplace_back(listOutConstraint1);
  intrnOpnds1.emplace_back(listInConstraint1);
  intrnOpnds1.emplace_back(listOutRegPrefix1);
  intrnOpnds1.emplace_back(listInRegPrefix1);
  maplebe::Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(mop1, intrnOpnds1);
  insn1.SetId(191);
  curBB->AppendInsn(insn1);

  // x0=[x1+0x40]                   (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop2 = MOP_xldr;
  RegOperand &r0 = aarFunc->GetOrCreatePhysicalRegisterOperand(R0, maplebe::k32BitSize, kRegTyInt);
  RegOperand &r1 = aarFunc->GetOrCreatePhysicalRegisterOperand(R1, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset2 = 64;
  MemOperand &memOpnd2 = aarFunc->CreateMemOpnd(r1, offset2, maplebe::k64BitSize);
  maplebe::Insn &insn2 = aarFunc->GetInsnBuilder()->BuildInsn(mop2, r0, memOpnd2);
  insn2.SetId(192);
  curBB->AppendInsn(insn2);

  // asm_operands                   nothing
  MOperator mop3 = MOP_asm;
  Operand &asmString2 = aarFunc->CreateStringOperand("");
  ListOperand *listInputOpnd2 = aarFunc->CreateListOpnd(*alloc);
  listInputOpnd2->PushOpnd(r0);
  ListOperand *listOutputOpnd2 = aarFunc->CreateListOpnd(*alloc);
  ListOperand *listClobber2 = aarFunc->CreateListOpnd(*alloc);
  auto *listInConstraint2 = memPool->New<ListConstraintOperand>(*alloc);
  auto *listOutConstraint2 = memPool->New<ListConstraintOperand>(*alloc);
  auto *listInRegPrefix2 = memPool->New<ListConstraintOperand>(*alloc);
  auto *listOutRegPrefix2 = memPool->New<ListConstraintOperand>(*alloc);
  std::vector<Operand*> intrnOpnds2;
  intrnOpnds2.emplace_back(&asmString2);
  intrnOpnds2.emplace_back(listOutputOpnd2);
  intrnOpnds2.emplace_back(listClobber2);
  intrnOpnds2.emplace_back(listInputOpnd2);
  intrnOpnds2.emplace_back(listOutConstraint2);
  intrnOpnds2.emplace_back(listInConstraint2);
  intrnOpnds2.emplace_back(listOutRegPrefix2);
  intrnOpnds2.emplace_back(listInRegPrefix2);
  maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, intrnOpnds2);
  insn3.SetId(193);
  curBB->AppendInsn(insn3);

  // x0=x0+0x40                     cortex_a53_slot_any
  MOperator mop4 = MOP_xaddrri12;
  ImmOperand &immOpnd4 = aarFunc->CreateImmOperand(64, maplebe::k64BitSize, false);
  maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, r0, r0, immOpnd4);
  insn4.SetId(194);
  curBB->AppendInsn(insn4);

  // asm_operands                   nothing
  MOperator mop5 = MOP_asm;
  maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, intrnOpnds2);
  insn5.SetId(195);
  curBB->AppendInsn(insn5);

  // x2=[x20++]                     (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop6 = MOP_xldr;
  RegOperand &r2 = aarFunc->GetOrCreatePhysicalRegisterOperand(R2, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r20 = aarFunc->GetOrCreatePhysicalRegisterOperand(R20, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset6 = 8;
  MemOperand &memOpnd6 = aarFunc->CreateMemOpnd(r20, offset6, maplebe::k64BitSize);
  memOpnd6.SetAddrMode(maplebe::MemOperand::kPostIndex);
  maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, r2, memOpnd6);
  insn6.SetId(196);
  curBB->AppendInsn(insn6);

  // cc=cmp(x20,x22)                cortex_a53_slot_any
  MOperator mop7 = MOP_xcmprr;
  Operand &ccReg = aarFunc->GetOrCreateRflag();
  RegOperand &r22 = aarFunc->GetOrCreatePhysicalRegisterOperand(R22, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, ccReg, r20, r22);
  insn7.SetId(207);
  curBB->AppendInsn(insn7);

  // x0=zxn([x2+0x12])              (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop8 = MOP_wldrh;
  maple::int64 offset8 = 18;
  MemOperand &memOpnd8 = aarFunc->CreateMemOpnd(r2, offset8, maplebe::k64BitSize);
  maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildInsn(mop8, r0, memOpnd8);
  insn8.SetId(197);
  curBB->AppendInsn(insn8);

  // x0=x0<<0x7&0x7f80              cortex_a53_slot_any
  MOperator mop9 = MOP_wubfizrri5i5;
  ImmOperand &lsbOpnd9 = aarFunc->CreateImmOperand(7, maplebe::k32BitSize, false);
  ImmOperand &widthOpnd9 = aarFunc->CreateImmOperand(8, maplebe::k32BitSize, false);
  maplebe::Insn &insn9 = aarFunc->GetInsnBuilder()->BuildInsn(mop9, r0, r0, lsbOpnd9, widthOpnd9);
  insn9.SetId(199);
  curBB->AppendInsn(insn9);

  // x1=[x2+0x40]                   (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop10 = MOP_xldr;
  maple::int64 offset10 = 64;
  MemOperand &memOpnd10 = aarFunc->CreateMemOpnd(r2, offset10, maplebe::k64BitSize);
  maplebe::Insn &insn10 = aarFunc->GetInsnBuilder()->BuildInsn(mop10, r1, memOpnd10);
  insn10.SetId(200);
  curBB->AppendInsn(insn10);

  // x2=zxn([x2+0x10])              (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop11 = MOP_wldrh;
  maple::int64 offset11 = 16;
  MemOperand &memOpnd11 = aarFunc->CreateMemOpnd(r2, offset11, maplebe::k64BitSize);
  maplebe::Insn &insn11 = aarFunc->GetInsnBuilder()->BuildInsn(mop11, r2, memOpnd11);
  insn11.SetId(203);
  curBB->AppendInsn(insn11);

  // [x1+0x8e]=x2                   (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop12 = MOP_wstrh;
  maple::int64 offset12 = 142;
  MemOperand &memOpnd12 = aarFunc->CreateMemOpnd(r1, offset12, maplebe::k64BitSize);
  maplebe::Insn &insn12 = aarFunc->GetInsnBuilder()->BuildInsn(mop12, r2, memOpnd12);
  insn12.SetId(204);
  curBB->AppendInsn(insn12);

  // [x1+0x10]=x0                   (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop13 = MOP_wstr;
  maple::int64 offset13 = 16;
  MemOperand &memOpnd13 = aarFunc->CreateMemOpnd(r1, offset13, maplebe::k64BitSize);
  maplebe::Insn &insn13 = aarFunc->GetInsnBuilder()->BuildInsn(mop13, r0, memOpnd13);
  insn13.SetId(201);
  curBB->AppendInsn(insn13);

  // [x1+0x78]=x0                   (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop14 = MOP_wstr;
  maple::int64 offset14 = 120;
  MemOperand &memOpnd14 = aarFunc->CreateMemOpnd(r1, offset14, maplebe::k64BitSize);
  maplebe::Insn &insn14 = aarFunc->GetInsnBuilder()->BuildInsn(mop14, r0, memOpnd14);
  insn14.SetId(202);
  curBB->AppendInsn(insn14);

  // pc={(cc==0)?L489:pc}           (cortex_a53_slot_any+cortex_a53_branch)
  MOperator mop15 = MOP_beq;
  auto &labelOpnd = aarFunc->GetOrCreateLabelOperand(1);
  maplebe::Insn &insn15 = aarFunc->GetInsnBuilder()->BuildInsn(mop15, ccReg, labelOpnd);
  insn15.SetId(208);
  curBB->AppendInsn(insn15);

  aarFunc->SetIsAfterRegAlloc();
  aarFunc->SetHasAsm();

  // Prepare data for list scheduling
  CDGNode *cdgNode = mock.CreateCDGNode(*curBB);

  DataDepAnalysis *dda = mock.GetDataDepAnalysis();
  dda->Run(*cdgNode->GetRegion());

  for (auto *depNode : cdgNode->GetAllDataNodes()) {
    Insn *insn = depNode->GetInsn();
    if (insn->GetId() == 192) {
      auto sucLinkIt = insn->GetDepNode()->GetSuccsBegin();
      while (sucLinkIt != insn->GetDepNode()->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 194 || succInsn->GetId() == 195 || succInsn->GetId() == 200) {
          sucLinkIt = insn->GetDepNode()->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 194) {
      auto sucLinkIt = insn->GetDepNode()->GetSuccsBegin();
      while (sucLinkIt != insn->GetDepNode()->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 197) {
          sucLinkIt = insn->GetDepNode()->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    } else if (insn->GetId() == 195) {
      auto sucLinkIt = insn->GetDepNode()->GetSuccsBegin();
      while (sucLinkIt != insn->GetDepNode()->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 199) {
          sucLinkIt = insn->GetDepNode()->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
    }
  }

  // Init in region
  AArch64LocalSchedule *aarLS = mock.GetAArch64LocalSchedule();
  aarLS->InitInRegion(*cdgNode->GetRegion());
  // Do list scheduling
  aarLS->SetUnitTest(true);
  aarLS->DoLocalSchedule(*cdgNode);

  delete memPool;
  delete alloc;
}

// HpfRecvDirectOther block3 after RA
TEST(listSchedule, testListSchedulingInLocalBlock3Sched2) {
  Triple::GetTriple().Init();
  auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
  auto *alloc = new MapleAllocator(memPool);
  MockListScheduler mock(memPool, alloc);
  AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

  // Mock maple BB info using gcc BB info before gcc scheduling
  auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
  aarFunc->SetCurBB(*curBB);
  aarFunc->SetFirstBB(*curBB);

  // {[sp+0x10]=x19;[sp+0x18]=x20;} (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop1 = MOP_xstp;
  RegOperand &sp = aarFunc->GetOrCreatePhysicalRegisterOperand(RSP, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r19 = aarFunc->GetOrCreatePhysicalRegisterOperand(R19, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r20 = aarFunc->GetOrCreatePhysicalRegisterOperand(R20, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset1 = 16;
  MemOperand &memOpnd1 = aarFunc->CreateMemOpnd(sp, offset1, maplebe::k64BitSize);
  maplebe::Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(mop1, r19, r20, memOpnd1);
  insn1.SetId(414);
  curBB->AppendInsn(insn1);

  // {[x29+0x38]=x24;[x29+0x40]=x25;} (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop2 = MOP_xstp;
  RegOperand &r24 = aarFunc->GetOrCreatePhysicalRegisterOperand(R24, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r25 = aarFunc->GetOrCreatePhysicalRegisterOperand(R25, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset2 = 56;
  MemOperand &memOpnd2 = aarFunc->CreateMemOpnd(sp, offset2, maplebe::k64BitSize);
  maplebe::Insn &insn2 = aarFunc->GetInsnBuilder()->BuildInsn(mop2, r24, r25, memOpnd2);
  insn2.SetId(416);
  curBB->AppendInsn(insn2);

  // {[x29+0x48]=x26;[x29+0x50]=x27;} (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop3 = MOP_xstp;
  RegOperand &r26 = aarFunc->GetOrCreatePhysicalRegisterOperand(R26, maplebe::k64BitSize, kRegTyInt);
  RegOperand &r27 = aarFunc->GetOrCreatePhysicalRegisterOperand(R27, maplebe::k64BitSize, kRegTyInt);
  maple::int64 offset3 = 72;
  MemOperand &memOpnd3 = aarFunc->CreateMemOpnd(sp, offset3, maplebe::k64BitSize);
  maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, r26, r27, memOpnd3);
  insn3.SetId(417);
  curBB->AppendInsn(insn3);

  // x24=high(`g_contextPool')      cortex_a53_slot_any
  MOperator mop4 = MOP_xadrp;
  MIRSymbol scg4(0, kScopeGlobal);
  scg4.SetStorageClass(kScGlobal);
  scg4.SetSKind(kStVar);
  std::string strSCG4("g_contextPool");
  scg4.SetNameStrIdx(strSCG4);
  StImmOperand &stImmOpnd4 = aarFunc->CreateStImmOperand(scg4, 0, 0);
  maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, r24, stImmOpnd4);
  insn4.SetId(76);
  curBB->AppendInsn(insn4);

  // x25=high(`g_mpOpsTable')       cortex_a53_slot_any
  MOperator mop5 = MOP_xadrp;
  MIRSymbol scg5(0, kScopeGlobal);
  scg5.SetStorageClass(kScGlobal);
  scg5.SetSKind(kStVar);
  std::string strSCG5("g_mpOpsTable");
  scg5.SetNameStrIdx(strSCG5);
  StImmOperand &stImmOpnd5 = aarFunc->CreateStImmOperand(scg5, 0, 0);
  maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, r25, stImmOpnd5);
  insn5.SetId(158);
  curBB->AppendInsn(insn5);

  // x27=zxn(x1-0x1)                cortex_a53_slot_any
  MOperator mop6 = MOP_wsubrri12;
  RegOperand &r1 = aarFunc->GetOrCreatePhysicalRegisterOperand(R1, maplebe::k32BitSize, kRegTyInt);
  ImmOperand &immOpnd6 = aarFunc->CreateImmOperand(1, maplebe::k32BitSize, false);
  maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, r27, r1, immOpnd6);
  insn6.SetId(54);
  curBB->AppendInsn(insn6);

  // x24=unspec[[x24+low(`g_contextPool')]] 24 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop7 = MOP_xldr;
  OfstOperand &ofstOpnd7 = aarFunc->CreateOfstOpnd(*stImmOpnd4.GetSymbol(), stImmOpnd4.GetOffset(), stImmOpnd4.GetRelocs());
  MemOperand *memOpnd7 = aarFunc->CreateMemOperand(maplebe::k64BitSize, r24, ofstOpnd7, *stImmOpnd4.GetSymbol());
  maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, r24, *memOpnd7);
  insn7.SetId(77);
  curBB->AppendInsn(insn7);

  // x26=unspec[[x25+low(`g_mpOpsTable')]] 24 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop8 = MOP_xldr;
  OfstOperand &ofstOpnd8 = aarFunc->CreateOfstOpnd(*stImmOpnd5.GetSymbol(), stImmOpnd5.GetOffset(), stImmOpnd5.GetRelocs());
  MemOperand *memOpnd8 = aarFunc->CreateMemOperand(maplebe::k64BitSize, r25, ofstOpnd8, *stImmOpnd5.GetSymbol());
  maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildInsn(mop8, r26, *memOpnd8);
  insn8.SetId(159);
  curBB->AppendInsn(insn8);

  // x20=x21                        cortex_a53_slot_any
  MOperator mop9 = MOP_xmovrr;
  RegOperand &r21 = aarFunc->GetOrCreatePhysicalRegisterOperand(R21, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn9 = aarFunc->GetInsnBuilder()->BuildInsn(mop9, r20, r21);
  insn9.SetId(53);
  curBB->AppendInsn(insn9);

  // [x29+0x28]=x22                 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_store
  MOperator mop10 = MOP_xstr;
  RegOperand &r22 = aarFunc->GetOrCreatePhysicalRegisterOperand(R22, maplebe::k64BitSize, kRegTyInt);
  int offset10 = 40;
  MemOperand &memOpnd10 = aarFunc->CreateMemOpnd(sp, offset10, maplebe::k64BitSize);
  memOpnd10.SetAddrMode(maplebe::MemOperand::kBOI);
  maplebe::Insn &insn10 = aarFunc->GetInsnBuilder()->BuildInsn(mop10, r22, memOpnd10);
  insn10.SetId(415);
  curBB->AppendInsn(insn10);

  // x22=x21+0x8                    cortex_a53_slot_any
  MOperator mop11 = MOP_xaddrri12;
  ImmOperand &immOpnd11 = aarFunc->CreateImmOperand(8, maplebe::k64BitSize, false);
  maplebe::Insn &insn11 = aarFunc->GetInsnBuilder()->BuildInsn(mop11, r22, r21, immOpnd11);
  insn11.SetId(56);
  curBB->AppendInsn(insn11);

  // x22=zxn(x27)<<0x3+x22          cortex_a53_slot_any
  MOperator mop12 = MOP_xxwaddrrre;
  ExtendShiftOperand &uxtwOpnd = aarFunc->CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, 3, maplebe::k3BitSize);
  maplebe::Insn &insn12 = aarFunc->GetInsnBuilder()->BuildInsn(mop12, r22, r22, r27, uxtwOpnd);
  insn12.SetId(58);
  curBB->AppendInsn(insn12);

  // pc=L206                        (cortex_a53_slot_any+cortex_a53_branch)
  MOperator mop13 = MOP_xuncond;
  std::string label = "ut_test_ls";
  auto *target = memPool->New<LabelOperand>(label.c_str(), 1, *memPool);
  maplebe::Insn &insn13 = aarFunc->GetInsnBuilder()->BuildInsn(mop13, *target);
  insn13.SetId(487);
  curBB->AppendInsn(insn13);

  // Prepare data for list scheduling
  CDGNode *cdgNode = mock.CreateCDGNode(*curBB);

  DataDepAnalysis *dda = mock.GetDataDepAnalysis();
  dda->Run(*cdgNode->GetRegion());

  // mock mem dependency from 77 to 415
  for (auto *depNode : cdgNode->GetAllDataNodes()) {
    Insn *insn = depNode->GetInsn();
    if (insn->GetId() == 77) {
      auto sucLinkIt = insn->GetDepNode()->GetSuccsBegin();
      while (sucLinkIt != insn->GetDepNode()->GetSuccsEnd()) {
        DepNode &succNode = (*sucLinkIt)->GetTo();
        Insn *succInsn = succNode.GetInsn();
        if (succInsn->GetId() == 415) {
          sucLinkIt = insn->GetDepNode()->EraseSucc(sucLinkIt);
          DepLink *succLink = *sucLinkIt;
          succNode.ErasePred(*succLink);
        } else {
          ++sucLinkIt;
        }
      }
      break;
    }
  }

  insn3.SetId(12);
  insn10.SetId(19);
  insn7.SetId(16);
  insn8.SetId(17);
  insn12.SetId(21);
  insn9.SetId(18);

  // Init in region
  AArch64LocalSchedule *aarLS = mock.GetAArch64LocalSchedule();
  aarLS->InitInRegion(*cdgNode->GetRegion());
  // Do list scheduling
  aarLS->SetUnitTest(true);
  aarLS->DoLocalSchedule(*cdgNode);

  delete memPool;
  delete alloc;
}

// HpfRecvDirectOther block3 before RA
TEST(listSchedule, testListSchedulingInLocalBlock3Sched1) {
  Triple::GetTriple().Init();
  auto *memPool = new MemPool(memPoolCtrler, "ut_list_schedule_mp");
  auto *alloc = new MapleAllocator(memPool);
  MockListScheduler mock(memPool, alloc);
  AArch64CGFunc *aarFunc = mock.GetAArch64CGFunc();

  // Mock maple BB info using gcc BB info before gcc scheduling
  auto *curBB = memPool->New<maplebe::BB>(0, *alloc);
  aarFunc->SetCurBB(*curBB);
  aarFunc->SetFirstBB(*curBB);

  // r171=r186                      cortex_a53_slot_any
  MOperator mop1 = MOP_xmovrr;
  auto R186 = static_cast<regno_t>(186);
  RegOperand *reg186 = aarFunc->CreateVirtualRegisterOperand(R186, maplebe::k64BitSize, kRegTyInt);
  auto R171 = static_cast<regno_t>(171);
  RegOperand *reg171 = aarFunc->CreateVirtualRegisterOperand(R171, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn1 = aarFunc->GetInsnBuilder()->BuildInsn(mop1, *reg171, *reg186);
  insn1.SetId(53);
  curBB->AppendInsn(insn1);

  // r132=r187-0x1                  cortex_a53_slot_any
  MOperator mop2 = MOP_wsubrri12;
  auto R187 = static_cast<regno_t>(187);
  RegOperand *reg187 = aarFunc->CreateVirtualRegisterOperand(R187, maplebe::k64BitSize, kRegTyInt);
  auto R132 = static_cast<regno_t>(132);
  RegOperand *reg132 = aarFunc->CreateVirtualRegisterOperand(R132, maplebe::k32BitSize, kRegTyInt);
  ImmOperand &immOpnd2 = aarFunc->CreateImmOperand(1, maplebe::k32BitSize, false);
  maplebe::Insn &insn2 = aarFunc->GetInsnBuilder()->BuildInsn(mop2, *reg132, *reg187, immOpnd2);
  insn2.SetId(54);
  curBB->AppendInsn(insn2);

  // r131=zxn(r132)                 cortex_a53_slot_any
  MOperator mop3 = MOP_xuxtw64;
  auto R131 = static_cast<regno_t>(131);
  RegOperand *reg131 = aarFunc->CreateVirtualRegisterOperand(R131, maplebe::k64BitSize, kRegTyInt);
  maplebe::Insn &insn3 = aarFunc->GetInsnBuilder()->BuildInsn(mop3, *reg131, *reg132);
  insn3.SetId(55);
  curBB->AppendInsn(insn3);

  // r188=r186+0x8                  cortex_a53_slot_any
  MOperator mop4 = MOP_xaddrri12;
  auto R188 = static_cast<regno_t>(188);
  RegOperand *reg188 = aarFunc->CreateVirtualRegisterOperand(R188, maplebe::k64BitSize, kRegTyInt);
  ImmOperand &immOpnd4 = aarFunc->CreateImmOperand(8, maplebe::k64BitSize, false);
  maplebe::Insn &insn4 = aarFunc->GetInsnBuilder()->BuildInsn(mop4, *reg188, *reg186, immOpnd4);
  insn4.SetId(56);
  curBB->AppendInsn(insn4);

  // r180=zxn(r132)<<0x3+r188       cortex_a53_slot_any
  MOperator mop5 = MOP_xxwaddrrre;
  auto R180 = static_cast<regno_t>(180);
  RegOperand *reg180 = aarFunc->CreateVirtualRegisterOperand(R180, maplebe::k64BitSize, kRegTyInt);
  ExtendShiftOperand &uxtwOpnd = aarFunc->CreateExtendShiftOperand(ExtendShiftOperand::kUXTW, 3, maplebe::k3BitSize);
  maplebe::Insn &insn5 = aarFunc->GetInsnBuilder()->BuildInsn(mop5, *reg180, *reg188, *reg132, uxtwOpnd);
  insn5.SetId(58);
  curBB->AppendInsn(insn5);

  // r266=high(`g_contextPool')     cortex_a53_slot_any
  MOperator mop6 = MOP_xadrp;
  auto R266 = static_cast<regno_t>(266);
  RegOperand *reg266 = aarFunc->CreateVirtualRegisterOperand(R266, maplebe::k64BitSize, kRegTyInt);
  MIRSymbol scg6(0, kScopeGlobal);
  scg6.SetStorageClass(kScGlobal);
  scg6.SetSKind(kStVar);
  std::string strSCG6("g_contextPool");
  scg6.SetNameStrIdx(strSCG6);
  StImmOperand &stImmOpnd6 = aarFunc->CreateStImmOperand(scg6, 0, 0);
  maplebe::Insn &insn6 = aarFunc->GetInsnBuilder()->BuildInsn(mop6, *reg266, stImmOpnd6);
  insn6.SetId(76);
  curBB->AppendInsn(insn6);

  // r267=unspec[[r266+low(`g_contextPool')]] 24 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop7 = MOP_xldr;
  auto R267 = static_cast<regno_t>(267);
  RegOperand *reg267 = aarFunc->CreateVirtualRegisterOperand(R267, maplebe::k64BitSize, kRegTyInt);
  OfstOperand &ofstOpnd7 = aarFunc->CreateOfstOpnd(*stImmOpnd6.GetSymbol(), stImmOpnd6.GetOffset(), stImmOpnd6.GetRelocs());
  MemOperand *memOpnd7 = aarFunc->CreateMemOperand(maplebe::k64BitSize, *reg266, ofstOpnd7, *stImmOpnd6.GetSymbol());
  maplebe::Insn &insn7 = aarFunc->GetInsnBuilder()->BuildInsn(mop7, *reg267, *memOpnd7);
  insn7.SetId(77);
  curBB->AppendInsn(insn7);

  // r268=high(`g_mpOpsTable')      cortex_a53_slot_any
  MOperator mop8 = MOP_xadrp;
  auto R268 = static_cast<regno_t>(268);
  RegOperand *reg268 = aarFunc->CreateVirtualRegisterOperand(R268, maplebe::k64BitSize, kRegTyInt);
  MIRSymbol scg8(0, kScopeGlobal);
  scg8.SetStorageClass(kScGlobal);
  scg8.SetSKind(kStVar);
  std::string strSCG8("g_mpOpsTable");
  scg8.SetNameStrIdx(strSCG8);
  StImmOperand &stImmOpnd8 = aarFunc->CreateStImmOperand(scg8, 0, 0);
  maplebe::Insn &insn8 = aarFunc->GetInsnBuilder()->BuildInsn(mop8, *reg268, stImmOpnd8);
  insn8.SetId(158);
  curBB->AppendInsn(insn8);

  // r269=unspec[[r268+low(`g_mpOpsTable')]] 24 (cortex_a53_slot_any+cortex_a53_ls_agen),cortex_a53_load
  MOperator mop9 = MOP_xldr;
  auto R269 = static_cast<regno_t>(269);
  RegOperand *reg269 = aarFunc->CreateVirtualRegisterOperand(R269, maplebe::k64BitSize, kRegTyInt);
  OfstOperand &ofstOpnd9 = aarFunc->CreateOfstOpnd(*stImmOpnd6.GetSymbol(), stImmOpnd8.GetOffset(), stImmOpnd8.GetRelocs());
  MemOperand *memOpnd8 = aarFunc->CreateMemOperand(maplebe::k64BitSize, *reg268, ofstOpnd9, *stImmOpnd8.GetSymbol());
  maplebe::Insn &insn9 = aarFunc->GetInsnBuilder()->BuildInsn(mop9, *reg269, *memOpnd8);
  insn9.SetId(159);
  curBB->AppendInsn(insn9);

  // Set max VRegNO
  aarFunc->SetMaxVReg(270);

  // Prepare data for list scheduling
  CDGNode *cdgNode = mock.CreateCDGNode(*curBB);

  // Execute local schedule using list scheduling algorithm on mock BB
  mock.DoLocalScheduleForMockBB(*cdgNode);

  delete memPool;
  delete alloc;
}