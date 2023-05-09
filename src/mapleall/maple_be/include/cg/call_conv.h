/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_CALL_CONV_H
#define MAPLEBE_INCLUDE_CG_CALL_CONV_H

#include "types_def.h"
#include "becommon.h"
#include "isa.h"

namespace maplebe {
using namespace maple;

/* for specifying how a parameter is passed */
struct CCLocInfo {
  regno_t reg0 = 0;  /* 0 means parameter is stored on the stack */
  regno_t reg1 = 0;
  regno_t reg2 = 0;  /* can have up to 4 single precision fp registers */
  regno_t reg3 = 0;  /* for small structure return. */
  int32 memOffset = 0;
  int32 memSize = 0;
  uint32 fpSize = 0;
  uint32 numFpPureRegs = 0;
  uint8 regCount = 0;             /* number of registers <= 2 storing the return value */
  PrimType primTypeOfReg0;    /* the primitive type stored in reg0 */
  PrimType primTypeOfReg1;    /* the primitive type stored in reg1 */
  PrimType primTypeOfReg2;
  PrimType primTypeOfReg3;

  void Clear() {
    reg0 = kInvalidRegNO;
    reg1 = kInvalidRegNO;
    reg2 = kInvalidRegNO;
    reg3 = kInvalidRegNO;
    memOffset = 0;
    memSize = 0;
    fpSize = 0;
    numFpPureRegs = 0;
    regCount = 0;
    primTypeOfReg0 = PTY_begin;
    primTypeOfReg1 = PTY_begin;
    primTypeOfReg2 = PTY_begin;
    primTypeOfReg3 = PTY_begin;
  }

  uint8 GetRegCount() const {
    return regCount;
  }

  PrimType GetPrimTypeOfReg0() const {
    return primTypeOfReg0;
  }

  PrimType GetPrimTypeOfReg1() const {
    return primTypeOfReg1;
  }

  PrimType GetPrimTypeOfReg2() const {
    return primTypeOfReg2;
  }

  PrimType GetPrimTypeOfReg3() const {
    return primTypeOfReg3;
  }

  regno_t GetReg0() const {
    return reg0;
  }

  regno_t GetReg1() const {
    return reg1;
  }

  regno_t GetReg2() const {
    return reg2;
  }

  regno_t GetReg3() const {
    return reg3;
  }
};

class LmbcFormalParamInfo {
 public:
  LmbcFormalParamInfo(PrimType pType, uint32 ofst, uint32 sz)
      : type(nullptr), primType(pType), offset(ofst), onStackOffset(0), size(sz), regNO(0), vregNO(0), numRegs(0),
        fpSize(0), isReturn(false), isPureFloat(false), isOnStack(false), hasRegassign(false) {}

  ~LmbcFormalParamInfo() = default;

  MIRStructType *GetType() {
    return type;
  }
  void SetType(MIRStructType *ty) {
    type = ty;
  }
  PrimType GetPrimType() const {
    return primType;
  }
  void SetPrimType(PrimType pType) {
    primType = pType;
  }
  uint32 GetOffset() const {
    return offset;
  }
  void SetOffset(uint32 ofs) {
    offset = ofs;
  }
  uint32 GetOnStackOffset() const {
    return onStackOffset;
  }
  void SetOnStackOffset(uint32 ofs) {
    onStackOffset = ofs;
  }
  uint32 GetSize() const {
    return size;
  }
  void SetSize(uint32 sz) {
    size = sz;
  }
  regno_t GetRegNO() const {
    return regNO;
  }
  void SetRegNO(regno_t reg) {
    regNO = reg;
  }
  regno_t GetVregNO() const {
    return vregNO;
  }
  void SetVregNO(regno_t reg) {
    vregNO = reg;
  }
  uint32 GetNumRegs() const {
    return numRegs;
  }
  void SetNumRegs(uint32 num) {
    numRegs = num;
  }
  uint32 GetFpSize() const {
    return fpSize;
  }
  void SetFpSize(uint32 sz) {
    fpSize = sz;
  }
  bool IsReturn() const {
    return isReturn;
  }
  void SetIsReturn() {
    isReturn = true;
  }
  bool IsPureFloat() const {
    return isPureFloat;
  }
  void SetIsPureFloat() {
    isPureFloat = true;
  }
  bool IsInReg() const {
    return !isOnStack ;
  }
  bool IsOnStack() const {
    return isOnStack;
  }
  void SetIsOnStack() {
    isOnStack = true;
  }
  bool HasRegassign() const {
    return hasRegassign;
  }
  void SetHasRegassign() {
    hasRegassign = true;
  }
 private:
  MIRStructType *type;
  PrimType primType;
  uint32 offset;
  uint32 onStackOffset;   /* stack location if isOnStack */
  uint32 size;        /* size primtype or struct */
  regno_t regNO = 0;  /* param reg num or starting reg num if numRegs > 0 */
  regno_t vregNO = 0; /* if no explicit regassing from IR, create move from param reg */
  uint32 numRegs = 0; /* number of regs for struct param */
  uint32 fpSize = 0;  /* size of fp param if isPureFloat */
  bool isReturn;
  bool isPureFloat = false;
  bool isOnStack; /* large struct is passed by a copy on stack */
  bool hasRegassign;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CALL_CONV_H */
