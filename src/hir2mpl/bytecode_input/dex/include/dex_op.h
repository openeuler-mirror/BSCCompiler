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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_DEX_OP_H
#define HIR2MPL_BC_INPUT_INCLUDE_DEX_OP_H
#include <vector>
#include <list>
#include "bc_op_factory.h"
#include "bc_instruction.h"
#include "dex_file_util.h"
#include "bc_parser_base.h"
#include "fe_manager.h"
namespace maple {
namespace bc {
class DexOp : public BCInstruction {
 public:
  DexOp(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOp() = default;
  void SetVA(uint32 num);
  void SetVB(uint32 num);
  void SetWideVB(uint64 num);
  void SetArgs(const MapleList<uint32> &args);
  void SetVC(uint32 num);
  void SetVH(uint32 num);
  static std::string GetArrayElementTypeFromArrayType(const std::string &typeName);

 protected:
  void ParseImpl(BCClassMethod &method) override {
    (void) method;
  }
  // Should be removed after all instruction impled
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override {
    return std::list<UniqueFEIRStmt>();
  }

  virtual void SetVAImpl(uint32 num) {}
  virtual void SetVBImpl(uint32 num) {}
  virtual void SetWideVBImpl(uint64 num) {}
  virtual void SetArgsImpl(const MapleList<uint32> &args) {};
  virtual void SetVCImpl(uint32 num) {}
  virtual void SetVHImpl(uint32 num) {}

  StructElemNameIdx *structElemNameIdx = nullptr;
};

struct DexReg : public BCReg {
  uint32 dexLitStrIdx = UINT32_MAX;  // string idx of dex
  uint32 dexTypeIdx = UINT32_MAX; // type idx of dex
};

class DexOpNop : public DexOp {
 public:
  DexOpNop(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpNop() = default;
};

// 0x01: move vA, vB ~ 0x09: move-object/16 vAAAA, vBBBB
class DexOpMove : public DexOp {
 public:
  DexOpMove(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpMove() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetRegTypeInTypeInferImpl() override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
};

// 0x0a: move-result vAA ~ 0x0c: move-result-object vAA
class DexOpMoveResult : public DexOp {
 public:
  DexOpMoveResult(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpMoveResult() = default;
  void SetVATypeNameIdx(const GStrIdx &idx);
  DexReg GetVA() const;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetBCRegTypeImpl(const BCInstruction &inst) override;
  DexReg vA;
};

// 0x0c
class DexOpMoveException : public DexOp {
 public:
  DexOpMoveException(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpMoveException() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetBCRegTypeImpl(const BCInstruction &inst) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
};

// 0x0e ~ 0x11
class DexOpReturn : public DexOp {
 public:
  DexOpReturn(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpReturn() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  bool isReturnVoid = false;
};

// 0x12 ~ 0x19
class DexOpConst : public DexOp {
 public:
  DexOpConst(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpConst() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetWideVBImpl(uint64 num) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;

 private:
  DexReg vA;
};

// 0x1a ~ 0x1b
class DexOpConstString : public DexOp {
 public:
  DexOpConstString(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpConstString() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  uint32 fileIdx = 0;
  DexReg vA;
  MapleString strValue;
};

// 0x1c
class DexOpConstClass : public DexOp {
 public:
  DexOpConstClass(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpConstClass() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  uint32 dexTypeIdx = 0;
  GStrIdx mplTypeNameIdx;
};

// 0x1d ~ 0x1e
class DexOpMonitor : public DexOp {
 public:
  DexOpMonitor(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpMonitor() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
};

// 0x1f
class DexOpCheckCast : public DexOp {
 public:
  DexOpCheckCast(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpCheckCast() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vDef;
  uint32 targetDexTypeIdx = 0;
  GStrIdx targetTypeNameIdx;
};

// 0x20
class DexOpInstanceOf : public DexOp {
 public:
  DexOpInstanceOf(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInstanceOf() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  uint32 targetDexTypeIdx = 0;
  std::string typeName;
};

// 0x21
class DexOpArrayLength : public DexOp {
 public:
  DexOpArrayLength(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpArrayLength() = default;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
};

// 0x22
class DexOpNewInstance : public DexOp {
 public:
  DexOpNewInstance(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpNewInstance() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  bool isSkipNewString = false;
  // isRcPermanent is true means the rc annotation @Permanent is used
  bool isRcPermanent = false;
};

// 0x23
class DexOpNewArray : public DexOp {
 public:
  DexOpNewArray(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpNewArray() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  // isRcPermanent is true means the rc annotation @Permanent is used
  bool isRcPermanent = false;
};

// 0x24 ~ 0x25
class DexOpFilledNewArray : public DexOp {
 public:
  DexOpFilledNewArray(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpFilledNewArray() = default;
  GStrIdx GetReturnType() const;

 protected:
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetArgsImpl(const MapleList<uint32> &args) override;
  void ParseImpl(BCClassMethod &method) override;
  bool isRange = false;
  uint32 argsSize = 0;
  uint32 dexArrayTypeIdx = UINT32_MAX;
  GStrIdx arrayTypeNameIdx;
  GStrIdx elemTypeNameIdx;
  MapleList<uint32> argRegs;
  MapleVector<DexReg> vRegs;
};

// 0x26
class DexOpFillArrayData : public DexOp {
 public:
  DexOpFillArrayData(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpFillArrayData() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  const int8 *arrayData = nullptr;
  int32 offset = INT32_MAX;
  uint32 size = 0;
};

// 0x27
class DexOpThrow : public DexOp {
 public:
  DexOpThrow(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpThrow() = default;

 private:
  void SetVAImpl(uint32 num) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
};

// 0x28 ~ 0x2a
class DexOpGoto : public DexOp {
 public:
  DexOpGoto(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpGoto() = default;

 private:
  std::vector<uint32> GetTargetsImpl() const override;
  void SetVAImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  int32 offset = 0;
  uint32 target = 0;
};

// 0x2b ~ 0x2c
class DexOpSwitch : public DexOp {
 public:
  DexOpSwitch(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpSwitch() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::vector<uint32> GetTargetsImpl() const override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  bool isPacked = false;
  int32 offset = 0;
  DexReg vA;
  MapleMap<int32, std::pair<uint32, uint32>> keyTargetOPpcMap;
};

// 0x2d ~ 0x31
class DexOpCompare : public DexOp {
 public:
  DexOpCompare(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpCompare() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  Opcode GetOpcodeFromDexIns() const;
  DexReg vA;
  DexReg vB;
  DexReg vC;
};

// 0x32 ~ 0x37
class DexOpIfTest : public DexOp {
 public:
  DexOpIfTest(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpIfTest() = default;

 protected:
  std::vector<uint32> GetTargetsImpl() const override;
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  int32 offset = 0;
  uint32 target = 0;
};

// 0x38 ~ 0x3d
class DexOpIfTestZ : public DexOp {
 public:
  DexOpIfTestZ(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpIfTestZ() = default;

 private:
  std::vector<uint32> GetTargetsImpl() const override;
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  int32 offset = 0;
  uint32 target = 0;
};

// 0x3e ~ 0x43, 0x73, 0x79 ~ 0x7a, 0xe3 ~ 0x f9
class DexOpUnused : public DexOp {
 public:
  DexOpUnused(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpUnused() = default;
};

// 0x44 ~ 0x4a
class DexOpAget : public DexOp {
 public:
  DexOpAget(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpAget() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  void SetRegTypeInTypeInferImpl() override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  DexReg vC;
};

// 0x4a ~ 0x51
class DexOpAput : public DexOp {
 public:
  DexOpAput(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpAput() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  void SetRegTypeInTypeInferImpl() override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  DexReg vC;
};

// 0x52 ~ 0x58
class DexOpIget : public DexOp {
 public:
  DexOpIget(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpIget() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  uint32 index = UINT32_MAX;
};

// 0x59 ~ 0x5f
class DexOpIput : public DexOp {
 public:
  DexOpIput(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpIput() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  uint32 index = UINT32_MAX;
};

// 0x60 ~ 0x66
class DexOpSget : public DexOp {
 public:
  DexOpSget(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpSget() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  uint32 index = UINT32_MAX;
  GStrIdx containerNameIdx;
  DexReg vA;
  int32 dexFileHashCode = -1;
};

// 0x67 ~ 0x6d
class DexOpSput : public DexOp {
 public:
  DexOpSput(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpSput() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  uint32 index = UINT32_MAX;
  int32 dexFileHashCode = -1;
};

class DexOpInvoke : public DexOp {
 public:
  DexOpInvoke(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvoke() = default;
  std::string GetReturnType() const;

 protected:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  void SetArgsImpl(const MapleList<uint32> &args) override;
  void ParseImpl(BCClassMethod &method) override;
  void PrepareInvokeParametersAndReturn(const FEStructMethodInfo &feMethodInfo, FEIRStmtCallAssign &stmt) const;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  bool IsStatic() const;
  bool ReplaceStringFactory(BCReader::ClassElem &methodInfo, MapleList<uint32> &argRegNums);
  bool isStringFactory = false;
  uint32 argSize = 0;
  uint32 arg0VRegNum = UINT32_MAX;
  uint32 methodIdx = 0;
  MapleList<uint32> argRegs;
  MapleVector<DexReg> argVRegs;
  std::vector<std::string> retArgsTypeNames;
  DexReg retReg;
};

// 0x6e, 0x74
class DexOpInvokeVirtual : public DexOpInvoke {
 public:
  DexOpInvokeVirtual(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokeVirtual() = default;
};

// 0x6f, 0x75
class DexOpInvokeSuper : public DexOpInvoke {
 public:
  DexOpInvokeSuper(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokeSuper() = default;
};

// 0x70, 0x76
class DexOpInvokeDirect : public DexOpInvoke {
 public:
  DexOpInvokeDirect(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokeDirect() = default;
};

// 0x71, 0x77
class DexOpInvokeStatic : public DexOpInvoke {
 public:
  DexOpInvokeStatic(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokeStatic() = default;
};

// 0x72, 0x78
class DexOpInvokeInterface : public DexOpInvoke {
 public:
  DexOpInvokeInterface(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokeInterface() = default;
};

// 0x7b, 0x8f
class DexOpUnaryOp : public DexOp {
 public:
  DexOpUnaryOp(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpUnaryOp() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  static std::map<uint8, std::tuple<Opcode, GStrIdx, GStrIdx>> InitOpcodeMapForUnary();
  // map<dexOpcode, tuple<mirOpcode, typeNameA, typeNameB>>
  static inline std::map<uint8, std::tuple<Opcode, GStrIdx, GStrIdx>> &GetOpcodeMapForUnary() {
    static std::map<uint8, std::tuple<Opcode, GStrIdx, GStrIdx>> opcodeMapForUnary = InitOpcodeMapForUnary();
    return opcodeMapForUnary;
  }
  DexReg vA;
  DexReg vB;
  Opcode mirOp = OP_undef;
};

// 0x90, 0xaf
class DexOpBinaryOp : public DexOp {
 public:
  DexOpBinaryOp(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpBinaryOp() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  Opcode GetOpcodeFromDexIns() const;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vA;
  DexReg vB;
  DexReg vC;
};

// 0xb0, 0xcf
class DexOpBinaryOp2Addr : public DexOp {
 public:
  DexOpBinaryOp2Addr(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpBinaryOp2Addr() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  Opcode GetOpcodeFromDexIns() const;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  DexReg vDef;
  DexReg vA;
  DexReg vB;
};

// 0xd0 ~ 0xe2
class DexOpBinaryOpLit : public DexOp {
 public:
  DexOpBinaryOpLit(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpBinaryOpLit() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  void SetVCImpl(uint32 num) override;
  Opcode GetOpcodeFromDexIns() const;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  bool isLit8;  // 8 bits / 16 bits signed int constant
  union {
    int8 i8;
    int16 i16;
  } constValue;
  DexReg vA;
  DexReg vB;
};

// 0xfa ~ 0xfb
class DexOpInvokePolymorphic : public DexOpInvoke {
 public:
  DexOpInvokePolymorphic(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokePolymorphic() = default;

 protected:
  void SetVHImpl(uint32 num) override;
  void ParseImpl(BCClassMethod &method) override;
  std::list<UniqueFEIRStmt> EmitToFEIRStmtsImpl() override;
  bool isStatic = false;
  uint32 protoIdx = 0;
  std::string fullNameMpl;
  std::string protoName;
  uint32 callerClassID = UINT32_MAX;
};

// 0xfc ~ 0xfd
class DexOpInvokeCustom : public DexOp {
 public:
  DexOpInvokeCustom(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpInvokeCustom() = default;

 private:
  void SetVBImpl(uint32 num) override;
  void SetArgsImpl(const MapleList<uint32> &args) override;
  MapleList<uint32> argRegs;
  MapleVector<DexReg> argVRegs;
  uint32 callSiteIdx = 0;
};

// 0xfe
class DexOpConstMethodHandle : public DexOp {
 public:
  DexOpConstMethodHandle(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpConstMethodHandle() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  DexReg vA;
  uint32 mhIdx = 0;
};

// 0xff
class DexOpConstMethodType : public DexOp {
 public:
  DexOpConstMethodType(MapleAllocator &allocatorIn, uint32 pcIn, DexOpCode opcodeIn);
  ~DexOpConstMethodType() = default;

 private:
  void SetVAImpl(uint32 num) override;
  void SetVBImpl(uint32 num) override;
  DexReg vA;
  uint32 protoIdx = 0;
};

constexpr BCOpFactory::funcPtr dexOpGeneratorMap[] = {
#define OP(opcode, category, kind, wide, throwable) \
  BCOpFactory::BCOpGenerator<DexOp##category, DexOpCode, kDexOp##opcode, k##kind, wide, throwable>,
#include "dex_opcode.def"
#undef OP
};
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_DEX_OP_H
