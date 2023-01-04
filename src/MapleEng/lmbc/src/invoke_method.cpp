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
#include <cstdio>
#include <cmath>
#include <climits>
#include <gnu/lib-names.h>
#include "mvalue.h"
#include "mprimtype.h"
#include "mfunction.h"
#include "mexpression.h"
#include "opcodes.h"
#include "massert.h"

namespace maple {

int64 MVal2Int64(MValue &val) {
  switch (val.ptyp) {
    case PTY_i64:
      return val.x.i64;
    case PTY_i32:
      return (int64)val.x.i32;
    case PTY_i16:
      return (int64)val.x.i16;
    case PTY_i8:
      return (int64)val.x.i8;
    case PTY_u32:
      return (int64)val.x.u32;
    default:
      MASSERT(false, "MValue type %d for int64 conversion NYI", val.ptyp);
  }
}

bool IsZero(MValue& cond) {
  switch (cond.ptyp) {
    case PTY_u8:  return cond.x.u8  == 0;
    case PTY_u16: return cond.x.u16 == 0;
    case PTY_u32: return cond.x.u32 == 0;
    case PTY_u64: return cond.x.u64 == 0;
    case PTY_i8:  return cond.x.i8  == 0;
    case PTY_i16: return cond.x.i16 == 0;
    case PTY_i32: return cond.x.i32 == 0;
    case PTY_i64: return cond.x.i64 == 0;
    default: MASSERT(false, "IsZero type %d NYI", cond.ptyp);
  }
}

bool RegAssignZextOrSext(MValue& from, PrimType toTyp, MValue& to) {
  switch (toTyp) {
    case PTY_u8:
      switch (from.ptyp) {
        case PTY_u32: to.x.u8 = from.x.u32; break;  // special case as needed
        default: return false;
      }
      break;
    case PTY_u32: 
      switch (from.ptyp) {
        case PTY_u8:  to.x.u64 = from.x.u8;  break;
        case PTY_u16: to.x.u64 = from.x.u16; break;
        case PTY_i32: to.x.u64 = from.x.i32; break;
        default: return false;
      }
      break;
    case PTY_i32: 
      switch (from.ptyp) {
        case PTY_i8:  to.x.i64 = from.x.i8;  break;
        case PTY_i16: to.x.i64 = from.x.i16; break;
        case PTY_u32: to.x.i64 = from.x.u32; break;
        default: return false;
      }
      break;
    case PTY_u64:
      switch (from.ptyp) {
        case PTY_u8:  to.x.u64 = from.x.u8;  break;
        case PTY_u16: to.x.u64 = from.x.u16; break;
        case PTY_u32: to.x.u64 = from.x.u32; break;
        case PTY_i64: to.x.u64 = from.x.i64; break; // for large_stack.c
        default: return false;
      }
      break;
    case PTY_i64:
      switch (from.ptyp) {
        case PTY_i8:  to.x.i64 = from.x.i8;  break;
        case PTY_i16: to.x.i64 = from.x.i16; break;
        case PTY_i32: to.x.i64 = from.x.i32; break;
        default: return false;
      }
      break;
    case PTY_i16:
      switch (from.ptyp) {
        case PTY_i32: to.x.i16 = from.x.i32; break;
        case PTY_u16: to.x.i16 = from.x.u16; break;
        default: return false;
      }
      break;
    case PTY_u16:
      switch (from.ptyp) {
        case PTY_u32: to.x.u16 = from.x.i32; break;
        default: return false;
      }
      break;
    default:
      return false;
      break;
  }
  to.ptyp = toTyp;
  return true;
}

#define CASE_TOPTYP(toPtyp, toCtyp) \
 case PTY_##toPtyp: \
   if (cvtInt)  res.x.toPtyp = (toCtyp)fromInt;   else \
   if (cvtUint) res.x.toPtyp = (toCtyp)fromUint;  else \
   if (cvtf32)  res.x.toPtyp = (toCtyp)fromFloat; else \
   if (cvtf64)  res.x.toPtyp = (toCtyp)fromDouble;     \
   break;

MValue CvtType(MValue &opnd, PrimType toPtyp, PrimType fromPtyp) {
  MValue res;
  intptr_t fromInt;
  uintptr_t fromUint;
  float  fromFloat;
  double fromDouble;
  bool cvtInt  = false;
  bool cvtUint = false;
  bool cvtf32  = false;
  bool cvtf64  = false;

  if (opnd.ptyp == toPtyp) {
    return opnd;
  } 
  switch (fromPtyp) {
    case PTY_i8:  fromInt  = opnd.x.i8;   cvtInt = true; break;
    case PTY_i16: fromInt  = opnd.x.i16;  cvtInt = true; break;
    case PTY_i32: fromInt  = opnd.x.i32;  cvtInt = true; break;
    case PTY_i64: fromInt  = opnd.x.i64;  cvtInt = true; break;
    case PTY_u8:  fromUint = opnd.x.u8;   cvtUint= true; break;
    case PTY_u16: fromUint = opnd.x.u16;  cvtUint= true; break;
    case PTY_u32: fromUint = opnd.x.u32;  cvtUint= true; break;
    case PTY_u64: fromUint = opnd.x.u64;  cvtUint= true; break;
    case PTY_a64: fromUint = opnd.x.u64;  cvtUint= true; break;
    case PTY_ptr: fromUint = opnd.x.u64;  cvtUint= true; break;
    case PTY_f32: fromFloat = opnd.x.f32; cvtf32 = true; break;
    case PTY_f64: fromDouble= opnd.x.f64; cvtf64 = true; break;
    default: MASSERT(false, "OP_cvt from ptyp %d NYI", fromPtyp); break;
  }
  switch (toPtyp) {
    CASE_TOPTYP(i8,  int8)
    CASE_TOPTYP(i16, int16)
    CASE_TOPTYP(i32, int32)
    CASE_TOPTYP(i64, int64)
    CASE_TOPTYP(u8,  uint8)
    CASE_TOPTYP(u16, uint16)
    CASE_TOPTYP(u32, uint32)
    CASE_TOPTYP(u64, uint64)
    CASE_TOPTYP(f32, float)
    CASE_TOPTYP(f64, double)
    case PTY_a64:
      if (cvtInt)
        res.x.a64 = (uint8*)fromInt;
      else if (cvtUint)
        res.x.a64 = (uint8*)fromUint;
      else
        MASSERT(false, "OP_cvt: type %d to %d not supported", fromPtyp, toPtyp);
      break;
    default: MASSERT(false, "OP_cvt: type %d to %d NYI", fromPtyp, toPtyp);
  }
  res.ptyp = toPtyp;
  return res;
}


inline bool CompareFloat(float x, float y, float epsilon = 0.00000001f) {
  if (isinf(x) && isinf(y)) {
    return true;
  }
  return (fabs(x - y) < epsilon) ? true : false;
}

inline bool CompareDouble(double x, double y, double epsilon = 0.0000000000000001f) {
  if (isinf(x) && isinf(y)) {
    return true;
  }
  return (fabs(x - y) < epsilon) ? true : false;
}

void HandleFloatEq(Opcode op, PrimType opndType, MValue &res, MValue &op1, MValue &op2) {
  MASSERT(opndType == op1.ptyp && op1.ptyp == op2.ptyp, "Operand type mismatch %d %d", op1.ptyp, op2.ptyp);
  switch (op) {
    case OP_ne:
      if (opndType == PTY_f32) {
        res.x.i64 = !CompareFloat(op1.x.f32, op2.x.f32);
      } else if (opndType == PTY_f64) {
        res.x.i64 = !CompareDouble(op1.x.f64, op2.x.f64);
      } else {
        MASSERT(false, "Unexpected type");
      }
      break;
    case OP_eq:
      if (opndType == PTY_f32) {
        res.x.i64 = CompareFloat(op1.x.f32, op2.x.f32);
      } else if (opndType == PTY_f64) {
        res.x.i64 = CompareDouble(op1.x.f64, op2.x.f64);
      } else {
        MASSERT(false, "Unexpected type");
      }
      break;
    default:
      break;
  }
}

void LoadArgs(MFunction& func) {
  for (int i=0; i < func.info->formalsNum; ++i) {
    if (func.info->pos2Parm[i]->isPreg) {
      func.pRegs[func.info->pos2Parm[i]->storeIdx] = func.caller->callArgs[i];
    } else {
      func.formalVars[func.info->pos2Parm[i]->storeIdx] = func.caller->callArgs[i];
    }
  }
}

// Handle regassign agg %%retval0
// Return agg <= 16 bytes in %%retval0 and %%retval1
void HandleAggrRetval(MValue &rhs, MFunction *caller) { 
  MASSERT(rhs.aggSize <= 16, "regassign of agg >16 bytes to %%retval0");
  uint64 retval[2] = {0, 0};
  memcpy_s(retval, sizeof(retval), rhs.x.a64, rhs.aggSize);
  caller->retVal0.x.u64 = retval[0];
  caller->retVal0.ptyp  = PTY_agg;  // treat as PTY_u64 if aggSize <= 16
  caller->retVal0.aggSize = rhs.aggSize;
  if (rhs.aggSize > maplebe::k8ByteSize) {
    caller->retVal1.x.u64 = retval[1];
    caller->retVal1.ptyp  = PTY_agg;
    caller->retVal1.aggSize= rhs.aggSize;
  }
}

// Walk the Maple LMBC IR tree of a function and execute its statements.
MValue InvokeFunc(LmbcFunc* fn, MFunction *caller) {
  MValue retVal;
  MValue pregs[fn->numPregs];                 // func pregs (incl. func formals that are pregs)
  MValue formalVars[fn->formalsNumVars+1];    // func formals that are named vars
  alignas(maplebe::k8ByteSize) uint8 frame[fn->frameSize];      // func autovars
  MFunction mfunc(fn, caller, frame, pregs, formalVars);  // init func execution state

  static void* const labels[] = {
        &&label_OP_Undef,
#define OPCODE(base_node,dummy1,dummy2,dummy3) &&label_OP_##base_node,
#include "opcodes.def"
#undef OPCODE
        &&label_OP_Undef
  };

  LoadArgs(mfunc);
  uint8 buf[ALLOCA_MEMMAX];
  mfunc.allocaMem = buf;
//  mfunc.allocaMem = static_cast<uint8*>(alloca(ALLOCA_MEMMAX));
  StmtNode *stmt = mfunc.nextStmt;
  goto *(labels[stmt->op]);

label_OP_Undef:
  {
    MASSERT(false, "Hit OP_undef");
  }
label_OP_block:
  {
    stmt = static_cast<BlockNode*>(stmt)->GetFirst();
    mfunc.nextStmt = stmt;
    goto *(labels[stmt->op]);
  }
label_OP_iassignfpoff:
  {
    IassignFPoffNode* node = static_cast<IassignFPoffNode *>(stmt);
    int32 offset= node->GetOffset();
    BaseNode* rhs = node->GetRHS();
    MValue val    = EvalExpr(mfunc, rhs);
    PrimType ptyp = node->ptyp;
    mstore(mfunc.fp+offset, ptyp, val);
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_call:
  {
    CallNode *call = static_cast<CallNode*>(stmt);
    MValue callArgs[call->NumOpnds()]; // stack for callArgs
    mfunc.callArgs    = callArgs;
    mfunc.numCallArgs = call->NumOpnds();
    if (IsExtFunc(call->GetPUIdx(), *mfunc.info->lmbcMod)) {
      mfunc.CallExtFuncDirect(call);
    } else {
      mfunc.CallMapleFuncDirect(call);
    }
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_regassign:
  {
    RegassignNode* node = static_cast<RegassignNode*>(stmt);
    PregIdx regIdx = node->GetRegIdx();
    MValue rhs     = EvalExpr(mfunc, node->GetRHS());
    if (node->ptyp == rhs.ptyp) {
      MASSERT(regIdx != -kSregRetval1, "regassign to %%%%retval1");
      if (regIdx == -kSregRetval0) {
        if (node->ptyp != PTY_agg) {
          caller->retVal0 = rhs;
        } else {
          HandleAggrRetval(rhs, caller);
        }
      } else {
        MASSERT(regIdx < fn->numPregs, "regassign regIdx %d out of bound", regIdx);
        mfunc.pRegs[regIdx] = rhs;
      }
    } else {
      bool extended = false;
      if (regIdx == -kSregRetval0) {
        extended = RegAssignZextOrSext(rhs, node->ptyp, caller->retVal0);
      } else if (regIdx == -kSregRetval1) {
        extended = RegAssignZextOrSext(rhs, node->ptyp, caller->retVal1);
      } else {
        MASSERT(regIdx < fn->numPregs, "regassign regIdx %d out of bound", regIdx);
        extended = RegAssignZextOrSext(rhs, node->ptyp, mfunc.pRegs[regIdx]);
      }
      if (!extended) {
        if ((node->ptyp == PTY_a64 || node->ptyp == PTY_u64) &&
            (rhs.ptyp == PTY_a64  || rhs.ptyp == PTY_u64)) {
          mfunc.pRegs[regIdx] = rhs;
          mfunc.pRegs[regIdx].ptyp = node->ptyp;
        } else {
          mfunc.pRegs[regIdx] = CvtType(rhs, node->ptyp, rhs.ptyp);
        }
      }
    }
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_brfalse:
label_OP_brtrue:
  {
    CondGotoNode* node = static_cast<CondGotoNode*>(stmt);
    uint32 labelIdx = node->GetOffset(); (void)labelIdx;
    MValue cond = EvalExpr(mfunc, node->GetRHS());
    StmtNode* label = fn->labelMap[labelIdx];
    if (stmt->op == OP_brfalse && IsZero(cond))  stmt = label;
    if (stmt->op == OP_brtrue  && !IsZero(cond)) stmt = label;
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_label:
  // no-op
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_goto:
  {
    uint32 labelIdx = static_cast<GotoNode*>(stmt)->GetOffset();
    StmtNode* label = fn->labelMap[labelIdx];
    stmt = label;
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_return:
  return caller->retVal0;
label_OP_iassignoff:
  {
    IassignoffNode* node = static_cast<IassignoffNode*>(stmt);
    int32 offset = node->GetOffset();
    MValue addr = EvalExpr(mfunc, node->Opnd(0));
    MValue rhs  = EvalExpr(mfunc, node->Opnd(1));
    mstore(addr.x.a64 + offset, stmt->ptyp, rhs);
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_blkassignoff:
  {
    BlkassignoffNode* node = static_cast<BlkassignoffNode*>(stmt);
    int32 dstOffset = node->offset;
    int32 blkSize = node->blockSize;
    MValue dstAddr = EvalExpr(mfunc, node->Opnd(0));
    MValue srcAddr = EvalExpr(mfunc, node->Opnd(1));
    memcpy_s(dstAddr.x.a64 + dstOffset, blkSize, srcAddr.x.a64, blkSize);
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_icallproto:
  {
    IcallNode *icallproto = static_cast<IcallNode*>(stmt);
    MASSERT(icallproto->NumOpnds() > 0, "icallproto num operands is %ld", icallproto->NumOpnds());
    // alloc stack space for call args
    MValue callArgs[icallproto->NumOpnds()-1];
    mfunc.callArgs    = callArgs;
    mfunc.numCallArgs = icallproto->NumOpnds()-1;
    // assume func addr in opnd 0 is from addroffunc
    MValue fnAddr = EvalExpr(mfunc, icallproto->Opnd(0));
    FuncAddr *faddr = reinterpret_cast<FuncAddr*>(fnAddr.x.a64);
    if (faddr->isLmbcFunc) {
      mfunc.CallMapleFuncIndirect(icallproto, faddr->funcPtr.lmbcFunc);
    } else {
      mfunc.CallExtFuncIndirect(icallproto, faddr->funcPtr.nativeFunc);
    }
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_rangegoto:
  {
    RangeGotoNode *rgoto = static_cast<RangeGotoNode*>(stmt);
    int32 tagOffset = rgoto->GetTagOffset();
    MValue opnd = EvalExpr(mfunc, rgoto->Opnd(0));
    int64 tag  = MVal2Int64(opnd);
    uint32 labelIdx = rgoto->GetRangeGotoTableItem(tag - tagOffset).second;
    StmtNode *label = fn->labelMap[labelIdx];
    stmt = label;
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_igoto:
  {
    MValue opnd = EvalExpr(mfunc, stmt->Opnd(0));
    StmtNode *label = (StmtNode*)opnd.x.a64;
    stmt = label;
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);
label_OP_intrinsiccall:
  {
    mfunc.CallIntrinsic(*static_cast<IntrinsiccallNode*>(stmt));
  }
  stmt = stmt->GetNext();
  mfunc.nextStmt = stmt;
  goto *(labels[stmt->op]);

label_OP_dassign:
label_OP_piassign:
label_OP_maydassign:
label_OP_iassign:
label_OP_doloop:
label_OP_dowhile:
label_OP_if:
label_OP_while:
label_OP_switch:
label_OP_multiway:
label_OP_foreachelem:
label_OP_comment:
label_OP_eval:
label_OP_free:
label_OP_calcassertge:
label_OP_calcassertlt:
label_OP_assertge:
label_OP_assertlt:
label_OP_callassertle:
label_OP_returnassertle:
label_OP_assignassertle:
label_OP_abort:
label_OP_assertnonnull:
label_OP_assignassertnonnull:
label_OP_callassertnonnull:
label_OP_returnassertnonnull:
label_OP_dread:
label_OP_iread:
label_OP_addrof:
label_OP_iaddrof:
label_OP_sizeoftype:
label_OP_fieldsdist:
label_OP_array:
label_OP_virtualcall:
label_OP_superclasscall:
label_OP_interfacecall:
label_OP_customcall:
label_OP_polymorphiccall:
label_OP_icall:
label_OP_interfaceicall:
label_OP_virtualicall:
label_OP_intrinsiccallwithtype:
label_OP_xintrinsiccall:
label_OP_callassigned:
label_OP_virtualcallassigned:
label_OP_superclasscallassigned:
label_OP_interfacecallassigned:
label_OP_customcallassigned:
label_OP_polymorphiccallassigned:
label_OP_icallassigned:
label_OP_interfaceicallassigned:
label_OP_virtualicallassigned:
label_OP_intrinsiccallassigned:
label_OP_intrinsiccallwithtypeassigned:
label_OP_xintrinsiccallassigned:
label_OP_callinstant:
label_OP_callinstantassigned:
label_OP_virtualcallinstant:
label_OP_virtualcallinstantassigned:
label_OP_superclasscallinstant:
label_OP_superclasscallinstantassigned:
label_OP_interfacecallinstant:
label_OP_interfacecallinstantassigned:
label_OP_jstry:
label_OP_try:
label_OP_cpptry:
label_OP_throw:
label_OP_jscatch:
label_OP_catch:
label_OP_cppcatch:
label_OP_finally:
label_OP_cleanuptry:
label_OP_endtry:
label_OP_safe:
label_OP_endsafe:
label_OP_unsafe:
label_OP_endunsafe:
label_OP_gosub:
label_OP_retsub:
label_OP_syncenter:
label_OP_syncexit:
label_OP_decref:
label_OP_incref:
label_OP_decrefreset:
label_OP_membaracquire:
label_OP_membarrelease:
label_OP_membarstoreload:
label_OP_membarstorestore:
label_OP_ireadoff:
label_OP_ireadfpoff:
label_OP_regread:
label_OP_addroffunc:
label_OP_addroflabel:
label_OP_constval:
label_OP_conststr:
label_OP_conststr16:
label_OP_ceil:
label_OP_cvt:
label_OP_floor:
label_OP_retype:
label_OP_round:
label_OP_trunc:
label_OP_abs:
label_OP_bnot:
label_OP_lnot:
label_OP_neg:
label_OP_recip:
label_OP_sqrt:
label_OP_sext:
label_OP_zext:
label_OP_alloca:
label_OP_malloc:
label_OP_gcmalloc:
label_OP_gcpermalloc:
label_OP_stackmalloc:
label_OP_gcmallocjarray:
label_OP_gcpermallocjarray:
label_OP_stackmallocjarray:
label_OP_resolveinterfacefunc:
label_OP_resolvevirtualfunc:
label_OP_add:
label_OP_sub:
label_OP_mul:
label_OP_div:
label_OP_rem:
label_OP_ashr:
label_OP_lshr:
label_OP_shl:
label_OP_ror:
label_OP_max:
label_OP_min:
label_OP_band:
label_OP_bior:
label_OP_bxor:
label_OP_CG_array_elem_add:
label_OP_eq:
label_OP_ge:
label_OP_gt:
label_OP_le:
label_OP_lt:
label_OP_ne:
label_OP_cmp:
label_OP_cmpl:
label_OP_cmpg:
label_OP_land:
label_OP_lior:
label_OP_cand:
label_OP_cior:
label_OP_select:
label_OP_intrinsicop:
label_OP_intrinsicopwithtype:
label_OP_extractbits:
label_OP_depositbits:
label_OP_iassignpcoff:
label_OP_ireadpcoff:
label_OP_checkpoint:
label_OP_addroffpc:
label_OP_asm:
label_OP_dreadoff:
label_OP_addrofoff:
label_OP_dassignoff:
label_OP_iassignspoff:
label_OP_icallprotoassigned:
  {
    MASSERT(false, "NIY");
    for(;;);
  }
  return retVal;
}

MValue EvalExpr(MFunction& func, BaseNode* expr, ParmInf *parm) {
  MValue res;

  static void* const labels[] = {
        &&label_OP_Undef,
#define OPCODE(base_node,dummy1,dummy2,dummy3) &&label_OP_##base_node,
#include "opcodes.def"
#undef OPCODE
        &&label_OP_Undef
  };

  goto *(labels[expr->op]);
label_OP_Undef:
  {
    MASSERT(false, "Hit OP_undef");
  }
label_OP_constval:
  {
     MIRConst* constval = static_cast<ConstvalNode*>(expr)->GetConstVal();
     int64  constInt = 0;
     float  constFloat = 0;
     double constDouble = 0;
     switch (constval->GetKind()) {
       case kConstInt:
         constInt = static_cast<MIRIntConst *>(constval)->GetExtValue();
         break;
       case kConstDoubleConst:
         constDouble = static_cast<MIRDoubleConst *>(constval)->GetValue();
         break;
       case kConstFloatConst:
         constFloat = static_cast<MIRFloatConst *>(constval)->GetValue();
         break;
       default:
         MASSERT(false, "constval kind %d NYI", constval->GetKind());
         break;
     }
     PrimType ptyp = expr->ptyp;
     switch (ptyp) {
       case PTY_i8:
       case PTY_i16:
       case PTY_i32:
       case PTY_i64:
         MASSERT(constval->GetKind() == kConstInt, "ptyp and constval kind mismatch");
         res.x.i64 = constInt;
         res.ptyp  = ptyp;
         break;
       case PTY_u8:
       case PTY_u16:
       case PTY_u32:
       case PTY_u64:
       case PTY_a64:
         MASSERT(constval->GetKind() == kConstInt, "ptyp and constval kind mismatch");
         res.x.u64 = constInt;
         res.ptyp  = ptyp;
         break;
       case PTY_f32:
         MASSERT(constval->GetKind() == kConstFloatConst, "ptyp and constval kind mismatch");
         res.x.f32 = constFloat;
         res.ptyp  = ptyp;
         break;
       case PTY_f64:
         MASSERT(constval->GetKind() == kConstDoubleConst, "constval ptyp and kind mismatch");
         res.x.f64 = constDouble;
         res.ptyp  = ptyp;
         break;
       default:
         MASSERT(false, "ptype %d for constval NYI", ptyp);
         break;
     }
  }
  goto _exit;

label_OP_add:
  {
    MValue op0 = EvalExpr(func, expr->Opnd(0));
    MValue op1 = EvalExpr(func, expr->Opnd(1));
    switch (expr->ptyp) {
        case PTY_i8:  res.x.i8  = op0.x.i8  + op1.x.i8;  break;
        case PTY_i16: res.x.i16 = op0.x.i16 + op1.x.i16; break;
        case PTY_i32: res.x.i32 = (int64)op0.x.i32 + (int64)op1.x.i32; break;
        case PTY_i64: res.x.i64 = (uint64)op0.x.i64 + (uint64)op1.x.i64; break;
        case PTY_u8:  res.x.u8  = op0.x.u8  + op1.x.u8;  break;
        case PTY_u16: res.x.u16 = op0.x.u16 + op1.x.u16; break;
        case PTY_u32: res.x.u32 = op0.x.u32 + op1.x.u32; break;
        case PTY_u64: res.x.u64 = op0.x.u64 + op1.x.u64; break;
        case PTY_a64: res.x.u64 = op0.x.u64 + op1.x.u64; break;
        case PTY_f32: res.x.f32 = op0.x.f32 + op1.x.f32; break;
        case PTY_f64: res.x.f64 = op0.x.f64 + op1.x.f64; break;
        default: MIR_FATAL("Unsupported PrimType %d for binary operator %s", expr->ptyp, "+");
    }
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_sub:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRBINOP(-, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_mul:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRBINOP(*, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_div:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRBINOP(/, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_regread:
  {
    PregIdx regIdx = static_cast<RegreadNode*>(expr)->GetRegIdx();
    switch (regIdx) {
      case -(kSregFp):
        MASSERT(expr->ptyp == PTY_a64, "regread %%FP with wrong ptyp %d", expr->ptyp);
        res.x.a64 = func.fp;
        break;
      case -(kSregRetval0):
        if (expr->ptyp == func.retVal0.ptyp) {
          res = func.retVal0;
        } else if (expr->ptyp == PTY_agg || expr->ptyp == PTY_u64) {
          res = func.retVal0;
        } else {
          res = CvtType(func.retVal0, expr->ptyp, func.retVal0.ptyp);
        }
        break;
      case -(kSregRetval1):
        MASSERT(expr->ptyp == func.retVal1.ptyp ||
                (expr->ptyp == PTY_agg || expr->ptyp == PTY_u64),
            "regread %%retVal0 type mismatch: %d %d", expr->ptyp, func.retVal1.ptyp);
        res = func.retVal1;
        break;
      case -(kSregGp):
        MASSERT(expr->ptyp == PTY_a64, "regread %%GP with wrong ptyp %d", expr->ptyp);
        res.x.a64 = func.info->lmbcMod->unInitPUStatics;
        break;
      default:
        MASSERT(regIdx < func.info->numPregs, "regread regIdx %d out of bound", regIdx);
        res = func.pRegs[regIdx];
        break;
    }
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_conststr:
  {
    UStrIdx ustrIdx = static_cast<ConststrNode*>(expr)->GetStrIdx();
    auto it = func.info->lmbcMod->globalStrTbl.insert(
        std::pair<uint32, std::string>(ustrIdx, GlobalTables::GetUStrTable().GetStringFromStrIdx(ustrIdx)));
    res.x.ptr = const_cast<char*>(it.first->second.c_str());
    res.ptyp  = PTY_a64;
    goto _exit;
  }
label_OP_ireadfpoff:
  {
    IreadFPoffNode* node = static_cast<IreadFPoffNode *>(expr);
    int32 offset = node->GetOffset();
    if (node->ptyp != PTY_agg) {
      mload(func.fp+offset, expr->ptyp, res);
      goto _exit;
    }
    // handle PTY_agg
    // ireadfpoff agg should be opnd from either call or return, i.e. either
    // - caller: call &foo (ireadfpoff agg -16, ireadfpoff agg -28)
    // - callee: regassign agg %%retval0 (ireadfpoff agg -12)
    // so set agg size in mvalue result with agg size from corresponding func formal or func return, and
    // - if from caller, read agg into caller allocated buffer for agg and return the pointer in an mvalue to be passed to callee
    // - if from callee, just pass fp offset ptr in an mvalue and regassign will copy it to caller's retval0/1 area
    if (func.nextStmt->op == OP_call) { // caller
      // check if calling external func:
      MASSERT(func.aggrArgsBuf != nullptr, "aggrArgsBuf is null");
      memcpy_s(func.aggrArgsBuf+parm->storeIdx, parm->size, func.fp+offset, parm->size);
      mload(func.aggrArgsBuf+parm->storeIdx, expr->ptyp, res, parm->size);
    } else {          // callee
      mload(func.fp+offset, expr->ptyp, res, func.info->retSize);
    }
    goto _exit;
  }
label_OP_ireadoff:
  {
    int32 offset  = static_cast<IreadoffNode *>(expr)->GetOffset();
    MValue rhs    = EvalExpr(func, static_cast<IreadoffNode *>(expr)->Opnd(0));
    if (expr->ptyp == PTY_agg) {
      // ireadoff agg should be operand for either call or return (like ireadfpoff)
      // - caller: call &f (ireadoff agg 0 (regread a64 %1))
      // - callee: regassign agg %%retval0 (ireadoff agg 0 (addrofoff a64 $x 0))
      MASSERT(rhs.ptyp == PTY_a64, "ireadoff agg RHS not PTY_agg");
      if (func.nextStmt->op == OP_call) {
        MASSERT(func.aggrArgsBuf != nullptr, "aggrArgsBuf is null");
        memcpy_s(func.aggrArgsBuf+parm->storeIdx, parm->size, rhs.x.a64 + offset, parm->size);
        mload(func.aggrArgsBuf+parm->storeIdx, expr->ptyp, res, parm->size);
      } else {
        MASSERT(func.nextStmt->op == OP_regassign, "ireadoff agg not used as regassign agg opnd");
        mload(rhs.x.a64 + offset, expr->ptyp, res, func.info->retSize);
      }
      goto _exit;
    }
    MASSERT(rhs.ptyp == PTY_a64 || rhs.ptyp == PTY_u64 || rhs.ptyp == PTY_i64,
        "ireadoff rhs ptyp %d not PTY_a64 or PTY_u64", rhs.ptyp);
    mload(rhs.x.a64+offset, expr->ptyp, res);
    goto _exit;
  }
label_OP_iread:
  {
    MASSERT(func.nextStmt->op == OP_call && expr->ptyp == PTY_agg, "iread unexpected outside call");
    MValue addr = EvalExpr(func, expr->Opnd(0));
    MASSERT(func.aggrArgsBuf != nullptr, "aggrArgsBuf is null");
    memcpy_s(func.aggrArgsBuf+parm->storeIdx, parm->size, addr.x.a64, parm->size);
    mload(func.aggrArgsBuf+parm->storeIdx, expr->ptyp, res, parm->size);
    goto _exit;
  }
label_OP_eq:
  {
    PrimType opndType = static_cast<CompareNode*>(expr)->GetOpndType();
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    if (opndType == PTY_f32 || opndType == PTY_f64) {
      res.ptyp  = expr->ptyp;
      HandleFloatEq(expr->op, opndType, res, opnd0, opnd1);
    } else {
      EXPRCOMPOPNOFLOAT(==, res, opnd0, opnd1, opndType, expr->ptyp);
    }
    goto _exit;
  }
label_OP_ne:
  {
    PrimType opndType = static_cast<CompareNode*>(expr)->GetOpndType();
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    if (opndType == PTY_f32 || opndType == PTY_f64) {
      res.ptyp = expr->ptyp;
      HandleFloatEq(expr->op, opndType, res, opnd0, opnd1);
    } else {
      EXPRCOMPOPNOFLOAT(!=, res, opnd0, opnd1, opndType, expr->ptyp);
    }
    goto _exit;
  }
label_OP_gt:
  {
    PrimType opndTyp= static_cast<CompareNode*>(expr)->GetOpndType();
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRCOMPOP(>, res, opnd0, opnd1, opndTyp, expr->ptyp);
    goto _exit;
  }
label_OP_ge:
  {
    PrimType opndTyp= static_cast<CompareNode*>(expr)->GetOpndType();
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRCOMPOP(>=, res, opnd0, opnd1, opndTyp, expr->ptyp);
    goto _exit;
  }
label_OP_lt:
  {
    PrimType opndTyp= static_cast<CompareNode*>(expr)->GetOpndType();
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRCOMPOP(<, res, opnd0, opnd1, opndTyp, expr->ptyp);
    goto _exit;
  }
label_OP_le:
  {
    PrimType opndTyp= static_cast<CompareNode*>(expr)->GetOpndType();
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRCOMPOP(<=, res, opnd0, opnd1, opndTyp, expr->ptyp);
    goto _exit;
  }
label_OP_select:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    MValue opnd2 = EvalExpr(func, expr->Opnd(2));
    EXPRSELECTOP(res, opnd0, opnd1, opnd2, expr->ptyp);
    goto _exit;
  }
label_OP_band:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRBININTOP(&, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_bior:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRBININTOP(|, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_bxor:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRBININTOP(^, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_lshr:
  {
    MValue opnd0   = EvalExpr(func, expr->Opnd(0));
    MValue numBits = EvalExpr(func, expr->Opnd(1));
    EXPRBININTOPUNSIGNED(>>, res, opnd0, numBits, expr->ptyp);
    goto _exit;
  }
label_OP_ashr:
  {
    MValue opnd0   = EvalExpr(func, expr->Opnd(0));
    MValue numBits = EvalExpr(func, expr->Opnd(1));
    EXPRBININTOP(>>, res, opnd0, numBits, expr->ptyp);
    goto _exit;
  }
label_OP_shl:
  {
    MValue opnd0   = EvalExpr(func, expr->Opnd(0));
    MValue numBits = EvalExpr(func, expr->Opnd(1));
    EXPRBININTOP(<<, res, opnd0, numBits, expr->ptyp);
    goto _exit;
  }
label_OP_bnot:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    EXPRUNROP(~, res, opnd0, expr->ptyp);
    goto _exit;
  }
label_OP_lnot:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    EXPRUNROP(!, res, opnd0, expr->ptyp);
    goto _exit;
  }
label_OP_neg:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    switch (expr->ptyp) {
      case PTY_i8:  res.x.i8  = -opnd0.x.i8;  break;
      case PTY_i16: res.x.i16 = -opnd0.x.i16; break;
      case PTY_i32: res.x.i32 = ~(uint32)opnd0.x.i32+1; break;
      case PTY_i64: res.x.i64 = -opnd0.x.i64; break;
      case PTY_u8:  res.x.u8  = -opnd0.x.u8;  break;
      case PTY_u16: res.x.u16 = -opnd0.x.u16; break;
      case PTY_u32: res.x.u32 = -opnd0.x.u32; break;
      case PTY_u64: res.x.u64 = -opnd0.x.u64; break;
      case PTY_f32: res.x.f32 = -opnd0.x.f32; break;
      case PTY_f64: res.x.f64 = -opnd0.x.f64; break;
      default: MIR_FATAL("Unsupported PrimType %d for unary operator %s", expr->ptyp, "OP_neg");
    }
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_abs:
  {
    MValue op0 = EvalExpr(func, expr->Opnd(0));
    switch (expr->ptyp) {
      // abs operand must be signed
      case PTY_i8:  res.x.i8  = abs(op0.x.i8);  break;
      case PTY_i16: res.x.i16 = abs(op0.x.i16); break;
      case PTY_i32: res.x.i32 = abs(op0.x.i32); break;
      case PTY_i64: res.x.i64 = abs(op0.x.i64); break;
      case PTY_f32: res.x.f32 = fabsf(op0.x.f32); break;
      case PTY_f64: res.x.f64 = fabs(op0.x.f64); break;
      default: MASSERT(false, "op_abs unsupported type %d", expr->ptyp);
    }
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_min:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRMAXMINOP(<, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_max:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRMAXMINOP(>, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_rem:
  {
    MValue opnd0 = EvalExpr(func, expr->Opnd(0));
    MValue opnd1 = EvalExpr(func, expr->Opnd(1));
    EXPRREMOP(%, res, opnd0, opnd1, expr->ptyp);
    goto _exit;
  }
label_OP_cvt:
  {
    PrimType toPtyp = expr->ptyp;
    PrimType fromPtyp = static_cast<TypeCvtNode*>(expr)->FromType();
    MValue opnd = EvalExpr(func, expr->Opnd(0));
    res = CvtType(opnd, toPtyp, fromPtyp);
    goto _exit;
  }
label_OP_addrofoff:
  {
    // get addr of var symbol which can be formal/global/extern/PUStatic
    int32 offset = static_cast<AddrofoffNode*>(expr)->offset;
    StIdx stidx  = static_cast<AddrofoffNode*>(expr)->stIdx;
    uint8 *addr;

    if (stidx.Islocal()) { // local sym can only be formals or PUstatic
      addr = func.GetFormalVarAddr(stidx);
      if (!addr) {
        addr = func.info->lmbcMod->GetVarAddr(func.info->mirFunc->GetPuidx(), stidx);
      }
      MASSERT(addr, "addrofoff can not find local var");
    } else {
      MASSERT(stidx.IsGlobal(), "addrofoff: symbol neither local nor global");
      MIRSymbol* var = GlobalTables::GetGsymTable().GetSymbolFromStidx(stidx.Idx());
      switch (var->GetStorageClass()) {
        case kScExtern:
          addr = (uint8 *)(func.info->lmbcMod->FindExtSym(stidx));
          break;
        case kScGlobal:
        case kScFstatic:
          addr = func.info->lmbcMod->GetVarAddr(var->GetStIdx());
          break;
        default:
          MASSERT(false, "addrofoff: storage class %d  NYI", var->GetStorageClass());
          break;
      }
    }
    res.x.a64 = addr + offset;
    res.ptyp = PTY_a64;
    goto _exit;
  }
label_OP_alloca:
  {
    MValue opnd =  EvalExpr(func, expr->Opnd(0));
    res.ptyp  = PTY_a64;
    res.x.a64 = func.Alloca(opnd.x.u64);
    goto _exit;
  }
label_OP_addroffunc:
  {
    FuncAddr *faddr = func.info->lmbcMod->GetFuncAddr(static_cast<AddroffuncNode*>(expr)->GetPUIdx());
    res.x.a64 = (uint8*)faddr;
    res.ptyp  = PTY_a64;
    goto _exit;
  }
label_OP_addroflabel:
  {
    AddroflabelNode *node = static_cast<AddroflabelNode*>(expr);
    LabelIdx labelIdx = node->GetOffset();
    StmtNode *label = func.info->labelMap[labelIdx];
    res.x.a64 = reinterpret_cast<uint8*>(label);
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_retype:
  {
    res = EvalExpr(func, expr->Opnd(0));
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_sext:
  {
    ExtractbitsNode *ext = static_cast<ExtractbitsNode*>(expr);
    uint8 bOffset = ext->GetBitsOffset();
    uint8 bSize   = ext->GetBitsSize();
    MASSERT(bOffset == 0, "sext unexpected offset");
    uint64 mask = bSize < k64BitSize ? (1ull << bSize) - 1 : ~0ull;
    res = EvalExpr(func, expr->Opnd(0));
    res.x.i64 = (((uint64)res.x.i64 >> (bSize - 1)) & 1ull) ? res.x.i64 | ~mask : res.x.i64 & mask;
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_zext:
  {
    ExtractbitsNode *ext = static_cast<ExtractbitsNode*>(expr);
    uint8 bOffset = ext->GetBitsOffset();
    uint8 bSize   = ext->GetBitsSize();
    MASSERT(bOffset == 0, "zext unexpected offset");
    uint64 mask = bSize < k64BitSize ? (1ull << bSize) - 1 : ~0ull;
    res = EvalExpr(func, expr->Opnd(0));
    res.x.i64 &= mask;
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_extractbits:
  {
    ExtractbitsNode *ebits = static_cast<ExtractbitsNode*>(expr);
    uint8 bOffset = ebits->GetBitsOffset();
    uint8 bSize   = ebits->GetBitsSize();
    uint64 mask = ((1ull << bSize) - 1) << bOffset;
    res  = EvalExpr(func, expr->Opnd(0));
    res.x.i64 = (uint64)(res.x.i64 & mask) >> bOffset;
    if (IsSignedInteger(expr->ptyp)) {
      mask = (1ull << bSize) - 1;
      res.x.i64 = (((uint64)res.x.i64 >> (bSize - 1)) & 1ull) ? res.x.i64 | ~mask : res.x.i64 & mask;
    }
    res.ptyp = expr->ptyp;
    goto _exit;
  }
label_OP_depositbits:
  {
    DepositbitsNode *dbits = static_cast<DepositbitsNode*>(expr);
    MValue opnd0  = EvalExpr(func, expr->Opnd(0));
    MValue opnd1  = EvalExpr(func, expr->Opnd(1));
    uint64 mask = ~(0xffffffffffffffff << dbits->GetBitsSize());
    uint64 from = (opnd1.x.u64 & mask) << dbits->GetBitsOffset();
    mask = mask << dbits->GetBitsOffset();
    res.x.u64 = (opnd0.x.u64 & ~(mask)) | from;
    res.ptyp  = expr->ptyp;
    goto _exit;
  }
label_OP_intrinsicop:
  {
    auto *intrnop = static_cast<IntrinsicopNode*>(expr);
    MValue op0 = EvalExpr(func, expr->Opnd(0));
    res.ptyp = expr->ptyp;
    switch (intrnop->GetIntrinsic()) {
      case INTRN_C_sin:
        if (expr->ptyp == PTY_f32) {
          res.x.f32 = sin(op0.x.f32);
        } else if (expr->ptyp == PTY_f64) {
          res.x.f64 = sin(op0.x.f64);
        }
        break;
      case INTRN_C_ctz32:
        if (expr->ptyp == PTY_u32 || expr->ptyp == PTY_i32) {
          res.x.u32 = __builtin_ctz(op0.x.u32);
        }
        break;
      case INTRN_C_clz32:
        if (expr->ptyp == PTY_u32 || expr->ptyp == PTY_i32) {
          res.x.u32 = __builtin_clz(op0.x.u32);
        }
        break;
      case INTRN_C_ffs:
        if (expr->ptyp == PTY_u32 || expr->ptyp == PTY_i32) {
          res.x.u32 = __builtin_ffs(op0.x.u32);
        }
        break;
      case INTRN_C_rev_4:
        if (expr->ptyp == PTY_u32 || expr->ptyp == PTY_i32) {
          res.x.u32 = __builtin_bitreverse32(op0.x.u32);
        }
        break;
      default:
        break;
    }
    goto _exit;
  }

  // unsupported opcodes
label_OP_dassign:
label_OP_piassign:
label_OP_maydassign:
label_OP_iassign:
label_OP_block:
label_OP_doloop:
label_OP_dowhile:
label_OP_if:
label_OP_while:
label_OP_switch:
label_OP_multiway:
label_OP_foreachelem:
label_OP_comment:
label_OP_eval:
label_OP_free:
label_OP_calcassertge:
label_OP_calcassertlt:
label_OP_assertge:
label_OP_assertlt:
label_OP_callassertle:
label_OP_returnassertle:
label_OP_assignassertle:
label_OP_abort:
label_OP_assertnonnull:
label_OP_assignassertnonnull:
label_OP_callassertnonnull:
label_OP_returnassertnonnull:
label_OP_dread:
label_OP_addrof:
label_OP_iaddrof:
label_OP_sizeoftype:
label_OP_fieldsdist:
label_OP_array:
label_OP_iassignoff:
label_OP_iassignfpoff:
label_OP_regassign:
label_OP_goto:
label_OP_brfalse:
label_OP_brtrue:
label_OP_return:
label_OP_rangegoto:
label_OP_call:
label_OP_virtualcall:
label_OP_superclasscall:
label_OP_interfacecall:
label_OP_customcall:
label_OP_polymorphiccall:
label_OP_icall:
label_OP_interfaceicall:
label_OP_virtualicall:
label_OP_intrinsiccall:
label_OP_intrinsiccallwithtype:
label_OP_xintrinsiccall:
label_OP_callassigned:
label_OP_virtualcallassigned:
label_OP_superclasscallassigned:
label_OP_interfacecallassigned:
label_OP_customcallassigned:
label_OP_polymorphiccallassigned:
label_OP_icallassigned:
label_OP_interfaceicallassigned:
label_OP_virtualicallassigned:
label_OP_intrinsiccallassigned:
label_OP_intrinsiccallwithtypeassigned:
label_OP_xintrinsiccallassigned:
label_OP_callinstant:
label_OP_callinstantassigned:
label_OP_virtualcallinstant:
label_OP_virtualcallinstantassigned:
label_OP_superclasscallinstant:
label_OP_superclasscallinstantassigned:
label_OP_interfacecallinstant:
label_OP_interfacecallinstantassigned:
label_OP_jstry:
label_OP_try:
label_OP_cpptry:
label_OP_throw:
label_OP_jscatch:
label_OP_catch:
label_OP_cppcatch:
label_OP_finally:
label_OP_cleanuptry:
label_OP_endtry:
label_OP_safe:
label_OP_endsafe:
label_OP_unsafe:
label_OP_endunsafe:
label_OP_gosub:
label_OP_retsub:
label_OP_syncenter:
label_OP_syncexit:
label_OP_decref:
label_OP_incref:
label_OP_decrefreset:
label_OP_membaracquire:
label_OP_membarrelease:
label_OP_membarstoreload:
label_OP_membarstorestore:
label_OP_label:
label_OP_conststr16:
label_OP_ceil:
label_OP_floor:
label_OP_round:
label_OP_trunc:
label_OP_recip:
label_OP_sqrt:
label_OP_malloc:
label_OP_gcmalloc:
label_OP_gcpermalloc:
label_OP_stackmalloc:
label_OP_gcmallocjarray:
label_OP_gcpermallocjarray:
label_OP_stackmallocjarray:
label_OP_resolveinterfacefunc:
label_OP_resolvevirtualfunc:
label_OP_ror:
label_OP_CG_array_elem_add:
label_OP_cmp:
label_OP_cmpl:
label_OP_cmpg:
label_OP_land:
label_OP_lior:
label_OP_cand:
label_OP_cior:
label_OP_intrinsicopwithtype:
label_OP_iassignpcoff:
label_OP_ireadpcoff:
label_OP_checkpoint:
label_OP_addroffpc:
label_OP_igoto:
label_OP_asm:
label_OP_dreadoff:
label_OP_dassignoff:
label_OP_iassignspoff:
label_OP_blkassignoff:
label_OP_icallproto:
label_OP_icallprotoassigned:
  {
    MASSERT(false, "NIY");
  }

_exit:
  return res;
}

} // namespace maple
