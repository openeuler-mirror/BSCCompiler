/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#ifndef MPLENG_LMBC_H_
#define MPLENG_LMBC_H_

#include <gnu/lib-names.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "mir_parser.h"
#include "bin_mplt.h"
#include "opcode_info.h"
#include "mir_function.h"
#include "constantfold.h"
#include "mir_type.h"
#include "mvalue.h"
#include "global_tables.h"
#include "mfunction.h"
#include "common_utils.h"

namespace maple {
class LmbcMod;

// Parameter and variable info
struct ParmInf {
  PrimType ptyp;
  size_t size;
  bool isPreg;
  bool isVararg;
  // use scenarios for storeIdx field:
  // ParmInf - idx into pReg array if isPreg (formal is preg)
  // ParmInf - idx into formalvar array if formal is var symbol
  // ParmInf - offset into func formalagg mem block if PTY_agg formal arg
  // ParmInf - offset into func varagg mem blk if PTY_agg var-arg
  // VarInf  - offset into module globals mem blk if global or PUStatic var
  int32 storeIdx;
  MIRSymbol *sym;  // VarInf only - for global and PUStatic var
  PUIdx puIdx;     // VarInf only - for PUStatic var
  ParmInf(PrimType type, size_t sz, bool ispreg, int32_t storageIdx) : ptyp(type),
      size(sz), isPreg(ispreg), storeIdx(storageIdx) {}
  ParmInf(PrimType type, size_t sz, bool ispreg, int32_t storageIdx, MIRSymbol *psym) : ptyp(type),
      size(sz), isPreg(ispreg), storeIdx(storageIdx), sym(psym) {}
  ParmInf(PrimType type, size_t sz, bool ispreg, int32_t storageIdx, MIRSymbol *psym, PUIdx puidx) : ptyp(type),
      size(sz), isPreg(ispreg), storeIdx(storageIdx), sym(psym), puIdx(puidx) {}
};

using LabelMap = std::unordered_map<LabelIdx, StmtNode*>;

class LmbcFunc {
 public:
  LmbcMod       *lmbcMod;
  MIRFunction   *mirFunc;
  uint32        retSize;
  uint32        frameSize;         // auto var size in bytes
  uint16        formalsNum;        // num formals: vars+pregs
  uint32        formalsNumVars;    // num formals: vars only
  uint32        formalsAggSize;    // total struct size of all formal args of type agg
  uint32        formalsSize;       // total size of all formal args
  LabelMap      labelMap;          // map labelIdx to Stmt address
  size_t        numPregs;
  bool          isVarArgs;
  std::vector<ParmInf*> pos2Parm;  // formals info lkup by pos order
  std::unordered_map<int32, ParmInf*> stidx2Parm;  // formals info lkup by formals stidx
  LmbcFunc(LmbcMod *mod, MIRFunction *func);
  void ScanFormals(void);
  void ScanLabels(StmtNode* stmt);
};

class FuncAddr {
 public:
  union {
    LmbcFunc *lmbcFunc;
    void *nativeFunc;
  } funcPtr;
  bool isLmbcFunc;
  uint32 formalsAggSize; // native func only
  std::string funcName;
  FuncAddr(bool lmbcFunc, void *func, std::string funcName, uint32 formalsAggSz = 0);
};

using VarInf = struct ParmInf;
using FuncMap = std::unordered_map<PUIdx, LmbcFunc*>;

class LmbcMod {
 public:
  std::string lmbcPath;
  MIRModule*  mirMod {nullptr};
  FuncMap     funcMap;
  LmbcFunc*   mainFn {nullptr};
  std::unordered_map<uint64, VarInf*> globalAndStaticVars;
  std::unordered_map<uint32, std::string> globalStrTbl;
  int         unInitPUStaticsSize {0};    // uninitalized PUStatic vars
  uint8*      unInitPUStatics{nullptr};
  int         globalsSize {0};            // global vars and initialized PUStatic
  uint8*      globals {nullptr};
  uint32      aggrInitOffset {0};

  void InitGlobalVars(void);
  void InitGlobalVariable(VarInf *pInf);
  void InitIntConst(VarInf *pInf, MIRIntConst &intConst, uint8 *dst);
  void InitStrConst(VarInf* pInf, MIRStrConst &mirStrConst, uint8 *dst);
  void InitAddrofConst(VarInf *pInf, MIRAddrofConst &addrofConst, uint8 *dst);
  void InitFloatConst(VarInf *pInf, MIRFloatConst &f32Const, uint8 *dst);
  void InitDoubleConst(VarInf *Inf, MIRDoubleConst &f64Const, uint8 *dst);
  void InitLblConst(VarInf *pInf, MIRLblConst &labelConst, uint8 *dst);
  void InitBitFieldConst(VarInf *pInf, MIRConst &mirConst, int32_t &allocdBits, bool &forceAlign);
  uint8_t* GetGlobalVarInitAddr(VarInf* pInf, uint32 align);
  void UpdateGlobalVarInitAddr(VarInf* pInf, uint32 size);
  void CheckUnamedBitField(MIRStructType &stType, uint32 &prevInitFd, uint32 curFd, int32 &allocdBits);

  LmbcMod(std::string path);
  MIRModule*  Import(std::string path);
  void        InitModule(void);
  void        CalcGlobalAndStaticVarSize(void);
  void        ScanPUStatic(MIRFunction *func);
  LmbcFunc*   LkupLmbcFunc(PUIdx puIdx);

  std::vector<void*> libHandles;
  std::unordered_map<PUIdx,  void*>extFuncMap;  // PUIdx to ext func addr map
  std::unordered_map<uint32, void*>extSymMap;   // StIdx.FullIdx() to ext sym addr map
  void  LoadDefLibs(void);
  void* FindExtFunc(PUIdx puidx);
  void* FindExtSym(StIdx stidx);
  void AddGlobalVar(MIRSymbol &sym, VarInf *pInf);
  void AddPUStaticVar(PUIdx puIdx, MIRSymbol &sym, VarInf *pInf);
  uint8 *GetVarAddr(StIdx stidx);               // globa var
  uint8 *GetVarAddr(PUIdx puidx, StIdx stidx);  // PUStatic var

  void InitAggConst(VarInf *pInf, MIRConst &mirConst);
  void InitArrayConst(VarInf *pInf, MIRConst &mirConst);
  void InitScalarConst(VarInf *pInf, MIRConst &mirConst);
  void InitPointerConst(VarInf *pInf, MIRConst &mirConst);
  std::unordered_map<PUIdx, FuncAddr*> PUIdx2FuncAddr;
  FuncAddr* GetFuncAddr(PUIdx puIdx);
};

} // namespace maple

#endif  // MPLENG_LMBC_H_
