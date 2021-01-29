/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_PREG_H
#define MAPLE_IR_INCLUDE_MIR_PREG_H
#if MIR_FEATURE_FULL
#include <climits>
#include "mir_module.h"
#include "global_tables.h"
#endif  // MIR_FEATURE_FULL

namespace maple {
extern void PrintIndentation(int32 );

// these special registers are encoded by negating the enumeration
enum SpecialReg : signed int {
  kSregSp = 1,
  kSregFp = 2,
  kSregGp = 3,
  kSregThrownval = 4,
  kSregMethodhdl = 5,
  kSregRetval0 = 6,
  kSregRetval1 = 7,
  kSregLast = 8,
};
#if MIR_FEATURE_FULL
class MIRPreg {
 public:
  explicit MIRPreg(uint32 n = 0) : MIRPreg(n, kPtyInvalid, nullptr) {}

  MIRPreg(uint32 n, PrimType ptyp) : primType(ptyp), pregNo(n) {}

  MIRPreg(uint32 n, PrimType ptyp, MIRType *mType) : primType(ptyp), pregNo(n), mirType(mType) {}

  ~MIRPreg() = default;
  void SetNeedRC(bool needRC = true) {
    this->needRC = needRC;
  }

  bool NeedRC() const {
    return needRC;
  }

  bool IsRef() const {
    return mirType != nullptr && primType == PTY_ref;
  }

  PrimType GetPrimType() const {
    return primType;
  }

  void SetPrimType(PrimType pty) {
    primType = pty;
  }

  int32 GetPregNo() const {
    return pregNo;
  }

  void SetPregNo(int32 pregNo) {
    this->pregNo = pregNo;
  }

  MIRType *GetMIRType() const {
    return mirType;
  }

  void SetMIRType(MIRType *mirType) {
    this->mirType = mirType;
  }

 private:
  PrimType primType = kPtyInvalid;
  bool needRC = false;
  int32 pregNo;  // the number in maple IR after the %
  MIRType *mirType;
};

class MIRPregTable {
 public:
  MIRPregTable(MIRModule *mod, MapleAllocator *allocator)
      : pregNoToPregIdxMap(allocator->Adapter()),
        pregTable(allocator->Adapter()),
        mAllocator(allocator) {
    pregTable.push_back(nullptr);
    specPregTable[0].SetPregNo(0);
    specPregTable[kSregSp].SetPregNo(-kSregSp);
    specPregTable[kSregFp].SetPregNo(-kSregFp);
    specPregTable[kSregGp].SetPregNo(-kSregGp);
    specPregTable[kSregThrownval].SetPregNo(-kSregThrownval);
    specPregTable[kSregMethodhdl].SetPregNo(-kSregMethodhdl);
    specPregTable[kSregRetval0].SetPregNo(-kSregRetval0);
    specPregTable[kSregRetval1].SetPregNo(-kSregRetval1);
    for (uint32 i = 0; i < kSregLast; ++i) {
      specPregTable[i].SetPrimType(PTY_unknown);
    }
  }

  ~MIRPregTable();

  PregIdx CreatePreg(PrimType primType, MIRType *mtype = nullptr) {
    ASSERT(!mtype || mtype->GetPrimType() == PTY_ref || mtype->GetPrimType() == PTY_ptr, "ref or ptr type");
    uint32 index = ++maxPregNo;
    MIRPreg *preg = mAllocator->GetMemPool()->New<MIRPreg>(index, primType, mtype);
    return AddPreg(preg);
  }

  PregIdx ClonePreg(MIRPreg *rfpreg) {
    PregIdx idx = CreatePreg(rfpreg->GetPrimType(), rfpreg->GetMIRType());
    MIRPreg *preg = pregTable[idx];
    preg->SetNeedRC(rfpreg->NeedRC());
    return idx;
  }

  MIRPreg *PregFromPregIdx(PregIdx pregidx) {
    if (pregidx < 0) {  // special register
      return &specPregTable[-pregidx];
    } else {
      return pregTable.at(pregidx);
    }
  }

  PregIdx GetPregIdxFromPregno(uint32 pregNo) {
    auto it = pregNoToPregIdxMap.find(pregNo);
    return (it == pregNoToPregIdxMap.end()) ? PregIdx(0) : it->second;
  }

  void DumpPregsWithTypes(int32 indent) {
    MapleVector<MIRPreg *> &pregtable = pregTable;
    for (uint32 i = 1; i < pregtable.size(); i++) {
      MIRPreg *mirpreg = pregtable[i];
      if (mirpreg->GetMIRType() == nullptr) {
        continue;
      }
      PrintIndentation(indent);
      LogInfo::MapleLogger() << "reg ";
      LogInfo::MapleLogger() << "%" << mirpreg->GetPregNo();
      LogInfo::MapleLogger() << " ";
      mirpreg->GetMIRType()->Dump(0);
      LogInfo::MapleLogger() << " " << (mirpreg->NeedRC() ? 1 : 0);
      LogInfo::MapleLogger() << "\n";
    }
  }

  size_t Size() const {
    return pregTable.size();
  }

  PregIdx AddPreg(MIRPreg *preg) {
    CHECK_FATAL(preg != nullptr, "invalid nullptr in AddPreg");
    PregIdx idx = static_cast<PregIdx>(pregTable.size());
    pregTable.push_back(preg);
    ASSERT(pregNoToPregIdxMap.find(preg->GetPregNo()) == pregNoToPregIdxMap.end(), "The same pregno is already taken");
    pregNoToPregIdxMap[preg->GetPregNo()] = idx;
    return idx;
  }

  PregIdx EnterPregNo(uint32 pregNo, PrimType ptyp, MIRType *ty = nullptr) {
    PregIdx idx = GetPregIdxFromPregno(pregNo);
    if (idx == 0) {
      if (pregNo > maxPregNo) {
        maxPregNo = pregNo;
      }
      MIRPreg *preg = mAllocator->GetMemPool()->New<MIRPreg>(pregNo, ptyp, ty);
      return AddPreg(preg);
    }
    return idx;
  }

  MapleVector<MIRPreg*> &GetPregTable() {
    return pregTable;
  }

  const MapleVector<MIRPreg*> &GetPregTable() const {
    return pregTable;
  }

  const MIRPreg *GetPregTableItem(const uint32 index) const {
    CHECK_FATAL(index < pregTable.size(), "array index out of range");
    return pregTable[index];
  }

  void SetPregNoToPregIdxMapItem(uint32 key, PregIdx value) {
    pregNoToPregIdxMap[key] = value;
  }

  uint32 GetMaxPregNo() const {
    return maxPregNo;
  }

  void SetMaxPregNo(uint32 index) {
    maxPregNo = index;
  }

 private:
  uint32 maxPregNo = 0;  //  the max pregNo that has been allocated
  MapleMap<uint32, PregIdx> pregNoToPregIdxMap;  // for quick lookup based on pregno
  MapleVector<MIRPreg*> pregTable;
  MIRPreg specPregTable[kSregLast];  // for the MIRPreg nodes corresponding to special registers
  MapleAllocator *mAllocator;
};

#endif  // MIR_FEATURE_FULL
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PREG_H
