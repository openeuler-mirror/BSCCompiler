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
#include <ffi.h>
#include "mfunction.h"
#include "mprimtype.h"
#include "massert.h"

namespace maple {

MFunction::MFunction(LmbcFunc  *funcInfo,
                     MFunction *funcCaller,
                     uint8     *autoVars,
                     MValue    *pregs,
                     MValue    *formalvars) :
    info(funcInfo),
    caller(funcCaller),
    frame(autoVars),
    pRegs(pregs),
    formalVars(formalvars),
    callArgs(nullptr),
    aggrArgsBuf(nullptr)
{
  numCallArgs = 0;
  nextStmt = info->mirFunc->GetBody();
  fp = (uint8*)frame + info->frameSize;
  allocaOffset = 0;
  allocaMem = nullptr;
}

MFunction::~MFunction() { }

uint8 *MFunction::Alloca(uint32 sz) {
  if (allocaOffset + sz > ALLOCA_MEMMAX) {
    return nullptr;
  }
  uint8 *ptr = allocaMem + allocaOffset;
  allocaOffset += sz;
  return ptr;
}

uint8 *MFunction::GetFormalVarAddr(StIdx stidx) {
  auto it = info->stidx2Parm.find(stidx.FullIdx());
  if (it == info->stidx2Parm.end()) {
    return nullptr;
  }
  if (it->second->ptyp == PTY_agg) {
    MASSERT(caller->aggrArgsBuf, "aggrArgsBuf not init");
    return caller->aggrArgsBuf + it->second->storeIdx;
  }
  return((uint8*)&formalVars[it->second->storeIdx].x);
}

bool IsExtFunc(PUIdx puIdx, LmbcMod& module) {
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  if (func->IsExtern() ||    // ext func with proto
      func->GetAttr(FUNCATTR_implicit) || // ext func with no proto
      !func->GetBody() ||    // func with no body
      (func->IsWeak() && module.FindExtFunc(puIdx))) {
    return true;
  }
  return false;
}

// Return size (roundup to 8 byte multiples) of aggregate returned by an iread.
size_t GetIReadAggrSize(BaseNode* expr) {
  MASSERT(expr->op == OP_iread && expr->ptyp == PTY_agg, "iread on non PTY_agg type");
  IreadNode *iread = static_cast<IreadNode*>(expr);
  TyIdx ptrTyIdx =  iread->GetTyIdx();
  MIRPtrType *ptrType =
      static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx));
  MIRType *aggType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrType->GetPointedTyIdx());
  FieldID fd = iread->GetFieldID();
  size_t sz = aggType->GetSize();
  if (fd != 0) {
    MIRStructType *structType = static_cast<MIRStructType*>(aggType);
    MIRType *fdType = structType->GetFieldType(fd);
    uint32 fdOffset = structType->GetBitOffsetFromBaseAddr(fd);
    sz = fdType->GetSize();
    (void)fdOffset;
  }
  return ((sz + 7) >> 3) << 3;
}

// Get total buffer size needed for all arguments of type PTY_agg in a call.
// This includes all type PTY_agg formal args and va-args (if any).
size_t GetAggCallArgsSize(LmbcFunc *callee, CallNode *call) {
  size_t totalAggCallArgsSize = callee->formalsAggSize;
  if (callee->isVarArgs) {
    for (int i = callee->formalsNum; i < call->NumOpnds(); i++) { // PTY_agg va-args
      if (call->Opnd(i)->ptyp == PTY_agg) {
        MASSERT(call->Opnd(i)->op == OP_iread, "agg var arg unexpected op");
        totalAggCallArgsSize += GetIReadAggrSize(call->Opnd(i));
      }
    }
  }
  return totalAggCallArgsSize;
}

// Caller passes arguments to callees using MValues which is a union of Maple
// prim types. For PTY_agg, MValue holds ptr to the aggregate data.
//
// Memory allocation scenarios for call arguments:
// 1. Non-varidiac callee:
//    - Caller alloca array of MValue to pass call operands to callee
//    - If any call arg is PTY_agg
//      - Caller alloca agg buffer to hold data for all args of type PTY_agg
//      - MValues for all args of type PTY_agg  points to offsets in agg buffer
// 2. Varidiac callee:
//    - Caller alloca array of MValue to pass call operands including va-arg ones to callee
//    - If any call arg is PTY_agg
//      - Caller alloca agg buffer to hold data for all args (formals+va_args) of type PTY_agg
//      - MValues of all args of type PTY_agg points to offsets in agg buffer
//    - Caller alloca arg stack per AARCH64 ABI for va-args and copy va-args to this arg stack
void MFunction::CallMapleFuncDirect(CallNode *call) {
  LmbcFunc *callee = info->lmbcMod->LkupLmbcFunc(call->GetPUIdx());
  if (!callee->formalsNum) {    // ignore args if callee takes no params
    InvokeFunc(callee, this);
    return;
  }
  // alloca stack space for aggr args before evaluating operands
  size_t totalAggCallArgsSize = GetAggCallArgsSize(callee, call);
  aggrArgsBuf = static_cast<uint8*>(alloca(totalAggCallArgsSize)); // stack alloc for aggr args
  for (int i = 0, sz = 0, offset = callee->formalsAggSize; i < call->NumOpnds(); i++) {
    // a non-aggregate arg
    if (call->Opnd(i)->ptyp != PTY_agg) {
      callArgs[i] = EvalExpr(*this, call->Opnd(i));
      continue;
    }
    // an aggregate formal arg
    if (i < callee->formalsNum) {
      callArgs[i] = EvalExpr(*this, call->Opnd(i), callee->pos2Parm[i]);
      continue;
    }
    // an aggregate var-arg
    sz = GetIReadAggrSize(call->Opnd(i));
    ParmInf parmInf(PTY_agg, sz, false, offset);
    offset += sz;
    callArgs[i] = EvalExpr(*this, call->Opnd(i), &parmInf);
  }
  if (callee->isVarArgs) {
    CallVaArgFunc(call->NumOpnds(), callee);
  } else {
    InvokeFunc(callee, this);
  }
}

void MFunction::CallMapleFuncIndirect(IcallNode *icall, LmbcFunc *callInfo) {
  if (!callInfo->formalsNum) {    // ignore caller args if callee has no formals
    InvokeFunc(callInfo, this);
    return;
  }

  // Set up call args - skip over 1st arg, which is addr of func to call
  aggrArgsBuf = static_cast<uint8*>(alloca(callInfo->formalsAggSize)); // stack alloc for aggr args
  for (int i=0; i < icall->NumOpnds()-1; i++) {
    callArgs[i] = (icall->Opnd(i+1)->ptyp == PTY_agg) ?
        EvalExpr(*this, icall->Opnd(i+1), callInfo->pos2Parm[i]) :
        EvalExpr(*this, icall->Opnd(i+1));
  }
  if (callInfo->isVarArgs) {
    CallVaArgFunc(icall->NumOpnds()-1, callInfo);
  } else {
    InvokeFunc(callInfo, this);
  }
}

// Maple front end generates Maple IR in varidiac functions to
// use target ABI va_list to access va-args. For the caller, this
// function sets up va-args in a stack buffer which is passed to
// the variadic callee in va_list fields (stack field in aarch64
// valist, or overflow_arg_area field in x86_64 valist):
// - Calc size of stack to emulate va-args stack in ABI
// - Alloca va-args stack and copy args to the stack
// - For vaArg up to 16 bytes, copy data directly to vaArg stack
// - For vaArg larger than 16 bytes, put pointer to data in vaArg stack
// - On a va_start intrinsic, the va-args stack addr in caller.vaArgs
//   is saved in corresponding vaList field for va-arg access by callee.
void MFunction::CallVaArgFunc(int numArgs, LmbcFunc *callInfo) {
  uint32 vArgsSz = 0;
  for (int i = callInfo->formalsNum; i < numArgs; ++i) {
    if (callArgs[i].ptyp != PTY_agg || callArgs[i].aggSize > 16 ) {
      vArgsSz += 8;
    } else {
      vArgsSz += callArgs[i].aggSize;
    } 
  }
  vaArgsSize = vArgsSz;
  vaArgs = static_cast<uint8*>(alloca(vaArgsSize));
  for (int i = callInfo->formalsNum, offset = 0; i < numArgs; ++i) {
    mstore(vaArgs + offset, callArgs[i].ptyp, callArgs[i], true);
    if (callArgs[i].ptyp != PTY_agg || callArgs[i].aggSize > 16 ) {
      offset += 8;
    } else {
      offset += callArgs[i].aggSize;
    } 
  }
  InvokeFunc(callInfo, this);
}

void MFunction::CallExtFuncDirect(CallNode* call) {
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(call->GetPUIdx());
  MapleVector<FormalDef> &formalDefVec = func->GetFormalDefVec();
  FuncAddr& faddr = *info->lmbcMod->GetFuncAddr(call->GetPUIdx());
  ffi_fp_t fp = (ffi_fp_t)(faddr.funcPtr.nativeFunc);
  MASSERT(fp, "External function not found");

  for (int i = formalDefVec.size(); i < call->NumOpnds(); i++) {
    if (call->Opnd(i)->ptyp == PTY_agg) {
      // TODO: Handle type PTY_agg va-args for external calls
      MASSERT(false, "extern func: va-arg of agg type NYI");
    }
  }
  // alloca stack space for aggr args before evaluating operands
  aggrArgsBuf = static_cast<uint8*>(alloca(faddr.formalsAggSize));
  for (int i = 0, offset = 0; i < call->NumOpnds(); i++) {
    // a non-aggregate arg
    if (call->Opnd(i)->ptyp != PTY_agg) {
      callArgs[i] = EvalExpr(*this, call->Opnd(i));
      continue;
    }
    // an aggregate formal arg
    if (i < formalDefVec.size()) {
      MIRType* ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(formalDefVec[i].formalTyIdx);
      MASSERT(ty->GetPrimType() == PTY_agg, "expects formal arg of agg type");
      ParmInf parmInf(PTY_agg, ty->GetSize(), false, offset);
      offset += ty->GetSize();
      callArgs[i] = EvalExpr(*this, call->Opnd(i), &parmInf);
      continue;
    }
    // TODO: Handle aggregate var-arg here. See CallMapleFuncDirect
  }
  CallWithFFI(func->GetReturnType()->GetPrimType(), fp);
}

void MFunction::CallExtFuncIndirect(IcallNode *icallproto, void* fp) {
  // TODO: handle aggregate args for ext funcs
  for (int i=0; i < icallproto->NumOpnds()-1; i++) {
    callArgs[i]= EvalExpr(*this, icallproto->Opnd(i+1));
  }
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(icallproto->GetRetTyIdx());
  MIRFuncType *fProto = static_cast<MIRFuncType*>(type);
  MIRType *fRetType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fProto->GetRetTyIdx());
  CallWithFFI(fRetType->GetPrimType(), (ffi_fp_t)fp);
}

void MFunction::CallIntrinsic(IntrinsiccallNode &intrn) {
  MIRIntrinsicID callId = intrn.GetIntrinsic();
  switch(callId) {
    case INTRN_C_va_start: {
      MValue addrofAP = EvalExpr(*this, intrn.Opnd(0));
      // setup VaList for Aarch64
      VaList *vaList = (maple::VaList*)addrofAP.x.a64;
      vaList->gr_offs = 0;    // indicate args are on vaList stack
      vaList->stack = caller->vaArgs;
      break;
    }
    default:
      MASSERT(false, "CallIntrinsic %d\n NYI", callId);
      break;
  }
}

// Maple PrimType to libffi type conversion table
static ffi_type ffi_type_table[] = {
        ffi_type_void, // kPtyInvalid
#define EXPANDFFI1(x) x,
#define EXPANDFFI2(x) EXPANDFFI1(x)
#define PRIMTYPE(P) EXPANDFFI2(FFITYPE_##P)
#define LOAD_ALGO_PRIMARY_TYPE
#include "prim_types.def"
#undef PRIMTYPE
        ffi_type_void // kPtyDerived
    };

// FFI type def for Arm64 VaList struct fields
ffi_type *vaListObjAarch64 [] = {
  ffi_type_table + PTY_ptr,
  ffi_type_table + PTY_ptr,
  ffi_type_table + PTY_ptr,
  ffi_type_table + PTY_i32,
  ffi_type_table + PTY_i32,
  nullptr
};

// FFI type def for X86_64 VaList struct fields
ffi_type *vaListObjX86_64 [] = {
  ffi_type_table + PTY_u32,
  ffi_type_table + PTY_u32,
  ffi_type_table + PTY_ptr,
  ffi_type_table + PTY_ptr,
  nullptr
};

// currently only support ARM64 va_list
ffi_type vaList_ffi_type = { 0, 0,  FFI_TYPE_STRUCT, vaListObjAarch64 };

// Setup array of call args and their types then call libffi interface
// to invoked external function. Note that for varidiac external funcs,
// PTY_agg type is used to detect a pass by value va_list arg, and used
// to setup its type for ffi call - this works for now because pass by
// value struct args (except va_list) is not supported yet when calling
// external varidiac functions - a different way to detect pass by value
// va_list arg will be needed when pass by value struct for external
// varidiac funcs is supported.
void MFunction::CallWithFFI(PrimType ret_ptyp, ffi_fp_t fp) {
  ffi_cif cif;
  ffi_type ffi_ret_type = ffi_type_table[ret_ptyp];
  ffi_type* arg_types[numCallArgs];
  void* args[numCallArgs];

  // gather args and arg types
  for (int i=0; i < numCallArgs; ++i) {
    args[i] = &callArgs[i].x;
    if (callArgs[i].ptyp == PTY_agg) {
      arg_types[i] = &vaList_ffi_type;
    } else {
      arg_types[i] = ffi_type_table + callArgs[i].ptyp;
    }
  }

  ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, numCallArgs, &ffi_ret_type, arg_types);
  if(status == FFI_OK) {
    ffi_call(&cif, fp, &retVal0.x, args);
    retVal0.ptyp = ret_ptyp;
  } else {
    MIR_FATAL("Failed to call method at %p", (void *)fp);
  }
}

} // namespace maple
