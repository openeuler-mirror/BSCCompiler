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
#ifndef MAPLEIR_INCLUDE_MIRSYMBOLBUILDER_H
#define MAPLEIR_INCLUDE_MIRSYMBOLBUILDER_H
#include <string>
#include <utility>
#include <vector>
#include <map>
#include "mir_builder.h"
#include "opcode_info.h"
#include "global_tables.h"

namespace maple {
class MIRSymbolBuilder {
 public:
  static MIRSymbolBuilder &Instance() {
    static MIRSymbolBuilder builder;
    return builder;
  }

  MIRSymbol *GetLocalDecl(const MIRSymbolTable &symbolTable, const GStrIdx &strIdx) const;
  MIRSymbol *CreateLocalDecl(MIRSymbolTable &symbolTable, GStrIdx strIdx, const MIRType &type) const;
  MIRSymbol *GetGlobalDecl(GStrIdx strIdx) const;
  MIRSymbol *CreateGlobalDecl(GStrIdx strIdx, const MIRType &type, MIRStorageClass sc) const;
  MIRSymbol *GetSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                       bool sameType = false) const;
  MIRSymbol *CreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                          MIRFunction *func, uint8 scpID) const;
  MIRSymbol *CreatePregFormalSymbol(TyIdx tyIdx, PregIdx pRegIdx, MIRFunction &func) const;
  size_t GetSymbolTableSize(const MIRFunction *func = nullptr) const;
  const MIRSymbol *GetSymbolFromStIdx(uint32 idx, const MIRFunction *func = nullptr) const;

 private:
  MIRSymbolBuilder() = default;
  ~MIRSymbolBuilder() = default;
  MIRSymbolBuilder(const MIRSymbolBuilder&) = delete;
  MIRSymbolBuilder(const MIRSymbolBuilder&&) = delete;
  MIRSymbolBuilder &operator=(const MIRSymbolBuilder&) = delete;
  MIRSymbolBuilder &operator=(const MIRSymbolBuilder&&) = delete;
};
} // maple
#endif // MAPLEIR_INCLUDE_MIRSYMBOLBUILDER_H
