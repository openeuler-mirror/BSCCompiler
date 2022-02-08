/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_VER_SYMBOL_H
#define MAPLE_ME_INCLUDE_VER_SYMBOL_H
#include <iostream>
#include "mir_module.h"
#include "mir_symbol.h"
#include "orig_symbol.h"

namespace maple {
class BB;  // circular dependency exists, no other choice
class PhiNode;  // circular dependency exists, no other choice
class MayDefNode;  // circular dependency exists, no other choice
class MustDefNode;  // circular dependency exists, no other choice
class VersionStTable;  // circular dependency exists, no other choice
class OriginalSt;  // circular dependency exists, no other choice

class VersionSt {
 public:
  enum DefType {
    kUnknown,
    kAssign,
    kPhi,
    kMayDef,
    kMustDef
  };

  VersionSt(size_t index, uint32 version, OriginalSt *ost)
      : index(index),
        version(version),
        ost(ost),
        defStmt() {}

  ~VersionSt() = default;

  size_t GetIndex() const {
    return index;
  }

  int GetVersion() const {
    return version;
  }

  void SetDefBB(BB *defbb) {
    defBB = defbb;
  }

  BB *GetDefBB() const {
    return defBB;
  }

  void DumpDefStmt(const MIRModule *mod) const;

  bool IsInitVersion() const {
    return version == kInitVersion;
  }

  DefType GetDefType() const {
    return defType;
  }

  void SetDefType(DefType defType) {
    this->defType = defType;
  }

  OStIdx GetOrigIdx() const {
    return ost->GetIndex();
  }

  OriginalSt *GetOst() const {
    return ost;
  }

  void SetOst(OriginalSt *originalSt) {
    this->ost = originalSt;
  }

  const StmtNode *GetAssignNode() const {
    return defStmt.assign;
  }

  StmtNode *GetAssignNode() {
    return defStmt.assign;
  }

  void SetAssignNode(StmtNode *assignNode) {
    defStmt.assign = assignNode;
  }

  const PhiNode *GetPhi() const {
    return defStmt.phi;
  }
  PhiNode *GetPhi() {
    return defStmt.phi;
  }
  void SetPhi(PhiNode *phiNode) {
    defStmt.phi = phiNode;
  }

  const MayDefNode *GetMayDef() const {
    return defStmt.mayDef;
  }
  MayDefNode *GetMayDef() {
    return defStmt.mayDef;
  }
  void SetMayDef(MayDefNode *mayDefNode) {
    defStmt.mayDef = mayDefNode;
  }

  const MustDefNode *GetMustDef() const {
    return defStmt.mustDef;
  }
  MustDefNode *GetMustDef() {
    return defStmt.mustDef;
  }
  void SetMustDef(MustDefNode *mustDefNode) {
    defStmt.mustDef = mustDefNode;
  }

  bool IsReturn() const {
    return isReturn;
  }

  void Dump(bool omitName = false) const {
    if (!omitName) {
      ost->Dump();
    }
    LogInfo::MapleLogger() << "(" << version << ")";
  }

  bool DefByMayDef() const {
    return defType == kMayDef;
  }

 private:
  size_t index;     // index number in versionst_table_
  int version;      // starts from 0 for each symbol
  OriginalSt *ost;  // the index of related originalst in originalst_table
  BB *defBB = nullptr;
  DefType defType = kUnknown;

  union DefStmt {
    StmtNode *assign;
    PhiNode *phi;
    MayDefNode *mayDef;
    MustDefNode *mustDef;
  } defStmt;  // only valid after SSA

  bool isReturn = false;  // the symbol will return in its function
};

class VersionStTable {
 public:
  explicit VersionStTable(MemPool &vstMp) : vstAlloc(&vstMp), versionStVector(vstAlloc.Adapter()) {
    versionStVector.push_back(static_cast<VersionSt*>(nullptr));
  }

  ~VersionStTable() = default;

  VersionSt *CreateNextVersionSt(OriginalSt *ost);

  void CreateZeroVersionSt(OriginalSt *ost);

  VersionSt *GetZeroVersionSt(const OriginalSt *ost) const {
    CHECK_FATAL(ost->GetVersionsIndices().size() != 0, "GetZeroVersionSt:: zero version has not been created");
    return versionStVector[ost->GetZeroVersionIndex()];
  }

  size_t GetVersionStVectorSize() const {
    return versionStVector.size();
  }

  VersionSt *GetVersionStVectorItem(size_t index) const {
    CHECK_FATAL(index < versionStVector.size(), "out of range");
    return versionStVector[index];
  }

  void SetVersionStVectorItem(size_t index, VersionSt *vst) {
    CHECK_FATAL(index < versionStVector.size(), "out of range");
    versionStVector[index] = vst;
  }

  MapleAllocator &GetVSTAlloc() {
    return vstAlloc;
  }

  void Dump(const MIRModule *mod) const;

 private:
  MapleAllocator vstAlloc;                   // this stores versionStVector
  MapleVector<VersionSt*> versionStVector;   // the vector that map a versionst's index to its pointer
};
}       // namespace maple
#endif  // MAPLE_ME_INCLUDE_VER_SYMBOL_H
