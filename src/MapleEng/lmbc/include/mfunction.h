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
#ifndef MPLENG_MFUNCTION_H_
#define MPLENG_MFUNCTION_H_

#include <vector>
#include <dlfcn.h>

#include <unistd.h>
#include <sys/syscall.h>

#include "mir_nodes.h"
#include "mvalue.h"
#include "lmbc_eng.h"

#define VARNAMELENGTH 16
#define ALLOCA_MEMMAX 0x4000

namespace maple {

class  LmbcMod;
class  LmbcFunc;
struct ParmInf;

using ffi_fp_t = void(*)();

// For x86 valist setup, to force va arg acess from stack pointed to
// by overflow_arg_area, set gp_offset to 48, fp_offset to 304, 
// reg_save_rea to null, and overflow_arg_arg to location for vaArgs.
typedef struct {
  uint gp_offset;
  uint fp_offset;
  void *overflow_arg_area;
  void *reg_save_area;
} VaListX86_64[1];

typedef struct {
  void *stack;
  void *gr_top;
  void *vr_top;
  int  gr_offs;
  int  vr_offs;
} VaListAarch64;

using VaList = VaListAarch64;

// State of executing Maple function
class MFunction {
 public:
    // state of an executing function
    LmbcFunc*   info;          // current func
    MFunction*  caller;        // caller of current func
    StmtNode*   nextStmt;      // next maple IR statement to execute
    uint8*      frame;         // stack frame (auto var only)
    uint8*      fp;            // point to bottom of frame
    uint8*      allocaMem;     // point to reserved stack memory for Maple IR OP_alloca
    uint32      allocaOffset;  // next avail offset in allocaMem
    MValue*     pRegs;         // array of  pseudo regs used in function
    MValue*     formalVars;    // array of var/non-preg args passed in

    // for function calls made from this function
    uint16      numCallArgs;   // number of call args to pass to callee
    MValue*     callArgs;      // array of call args to pass to callee
    uint8*      aggrArgsBuf;   // buffer for PTY_agg call formal args, which offsets into it
    uint8*      varArgsBuf;    // buffer for PTY_agg call var-args, which offsets into it
    MValue      retVal0;       // %retVal0 return from callee
    MValue      retVal1;       // %retval1 return from callee
    uint8*      vaArgs;        // AARCH64 ABI vararg stack for calling va-arg funcs
    uint32      vaArgsSize;

    explicit MFunction(LmbcFunc  *funcInfo,
                       MFunction *funcCaller,
                       uint8     *autoVars,
                       MValue    *pRegs,
                       MValue    *formalVars);
    ~MFunction();
    uint8 *Alloca(uint32 size);
    uint8 *GetFormalVarAddr(StIdx stidx);
    void CallMapleFuncDirect(CallNode *call);
    void CallMapleFuncIndirect(IcallNode *icall, LmbcFunc *callInfo);
    void CallExtFuncDirect(CallNode* call);
    void CallExtFuncIndirect(IcallNode *icallproto, void* fp);
    void CallVaArgFunc(int numArgs, LmbcFunc *callInfo);
    void CallWithFFI(PrimType ret_ptyp, ffi_fp_t fp);
    void CallIntrinsic(IntrinsiccallNode &intrn);
};

bool IsExtFunc(PUIdx puIdx, LmbcMod &module);
MValue InvokeFunc(LmbcFunc* fn, MFunction *caller);
MValue EvalExpr(MFunction &func, BaseNode* expr, ParmInf *parm = nullptr);
void mload(uint8* addr, PrimType ptyp, MValue& res, size_t aggSizea = 0);
void mstore(uint8* addr, PrimType ptyp, MValue& val, bool toVarArgStack = false);

}
#endif // MPLENG_MFUNCTION_H_
