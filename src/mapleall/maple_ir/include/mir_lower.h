/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_LOWER_H
#define MAPLE_IR_INCLUDE_MIR_LOWER_H
#include <iostream>
#include "mir_builder.h"
#include "opcodes.h"

namespace maple {
enum MirLowerPhase : uint8 {
  kLowerUnder,
  kLowerMe,
  kLowerExpandArray,
  kLowerBe,
  kLowerCG,
  kLowerLNO
};

constexpr uint32 kShiftLowerMe = 1U << kLowerMe;
constexpr uint32 kShiftLowerExpandArray = 1U << kLowerExpandArray;
constexpr uint32 kShiftLowerBe = 1U << kLowerBe;
constexpr uint32 kShiftLowerCG = 1U << kLowerCG;
constexpr uint32 kShiftLowerLNO = 1U << kLowerLNO;
// check if a block node ends with an unconditional jump
inline bool OpCodeNoFallThrough(Opcode opCode) {
  return opCode == OP_goto || opCode == OP_return || opCode == OP_switch || opCode == OP_throw || opCode == OP_gosub ||
         opCode == OP_retsub;
}

inline bool IfStmtNoFallThrough(const IfStmtNode &ifStmt) {
  return OpCodeNoFallThrough(ifStmt.GetThenPart()->GetLast()->GetOpCode());
}

class MIRLower {
 public:
  static const std::set<std::string> kSetArrayHotFunc;

  MIRLower(MIRModule &mod, MIRFunction *f) : mirModule(mod), mirFunc(f) {}

  virtual ~MIRLower() = default;

  const MIRFunction *GetMirFunc() const {
    return mirFunc;
  }

  void SetMirFunc(MIRFunction *f) {
    mirFunc = f;
  }

  void Init() {
    mirBuilder = mirModule.GetMemPool()->New<MIRBuilder>(&mirModule);
  }

  virtual BlockNode *LowerIfStmt(IfStmtNode &ifStmt, bool recursive);
  BlockNode *LowerSwitchStmt(SwitchNode *switchNode);
  virtual BlockNode *LowerWhileStmt(WhileStmtNode&);
  BlockNode *LowerDowhileStmt(WhileStmtNode&);
  BlockNode *LowerDoloopStmt(DoloopNode&);
  BlockNode *LowerBlock(BlockNode&);
  BaseNode *LowerEmbeddedCandCior(BaseNode *x, StmtNode *curstmt, BlockNode *block);
  void LowerCandCior(BlockNode &block);
  void LowerBuiltinExpect(BlockNode &block);
  void LowerFunc(MIRFunction &func);
  BaseNode *LowerFarray(ArrayNode *array);
  BaseNode *LowerCArray(ArrayNode *array);
  void ExpandArrayMrt(MIRFunction &func);
  IfStmtNode *ExpandArrayMrtIfBlock(IfStmtNode &node);
  WhileStmtNode *ExpandArrayMrtWhileBlock(WhileStmtNode &node);
  DoloopNode *ExpandArrayMrtDoloopBlock(DoloopNode &node);
  ForeachelemNode *ExpandArrayMrtForeachelemBlock(ForeachelemNode &node);
  BlockNode *ExpandArrayMrtBlock(BlockNode &block);
  void AddArrayMrtMpl(BaseNode &exp, BlockNode &newblk);
  void SetLowerME() {
    lowerPhase |= kShiftLowerMe;
  }

  void SetLowerLNO() {
    lowerPhase |= kShiftLowerLNO;
  }

  void SetLowerExpandArray() {
    lowerPhase |= kShiftLowerExpandArray;
  }

  void SetLowerBE() {
    lowerPhase |= kShiftLowerBe;
  }

  void SetLowerCG() {
    lowerPhase |= kShiftLowerCG;
  }

  uint8 GetOptLevel() {
    return optLevel;
  }

  void SetOptLevel(uint8 optlvl) {
    optLevel = optlvl;
  }

  bool IsLowerME() const {
    return lowerPhase & kShiftLowerMe;
  }

  bool IsLowerLNO() const {
    return lowerPhase & kShiftLowerLNO;
  }

  bool IsLowerExpandArray() const {
    return lowerPhase & kShiftLowerExpandArray;
  }

  bool IsLowerBE() const {
    return lowerPhase & kShiftLowerBe;
  }

  bool IsLowerCG() const {
    return lowerPhase & kShiftLowerCG;
  }

  static bool ShouldOptArrayMrt(const MIRFunction &func);

  virtual bool InLFO() const { return false; }

 protected:
  MIRModule &mirModule;
 private:
  MIRFunction *mirFunc;
  MIRBuilder *mirBuilder = nullptr;
  uint32 lowerPhase = 0;
  uint8 optLevel = 0;
  LabelIdx CreateCondGotoStmt(Opcode op, BlockNode &blk, const IfStmtNode &ifStmt);
  void CreateBrFalseStmt(BlockNode &blk, const IfStmtNode &ifStmt);
  void CreateBrTrueStmt(BlockNode &blk, const IfStmtNode &ifStmt);
  void CreateBrFalseAndGotoStmt(BlockNode &blk, const IfStmtNode &ifStmt);
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_LOWER_H
