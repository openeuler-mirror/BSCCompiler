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
#include "constantfold.h"
#include <cmath>
#include <cfloat>
#include <climits>
#include <type_traits>
#include "mpl_logging.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "global_tables.h"
#include "me_option.h"
#include "maple_phase_manager.h"

namespace {
constexpr maple::uint64 kJsTypeNumber = 4;
constexpr maple::uint64 kJsTypeNumberInHigh32Bit = kJsTypeNumber << 32; // set high 32 bit as JSTYPE_NUMBER
constexpr maple::uint32 kByteSizeOfBit64 = 8; // byte number for 64 bit
constexpr maple::uint32 kBitSizePerByte = 8;
enum CompareRes : maple::int64 {
  kLess = -1,
  kEqual = 0,
  kGreater = 1
};
}

namespace maple {
// This phase is designed to achieve compiler optimization by
// simplifying constant expressions. The constant expression
// is evaluated and replaced by the value calculated on compile
// time to save time on runtime.
//
// The main procedure shows as following:
// A. Analyze expression type
// B. Analysis operator type
// C. Replace the expression with the result of the operation
BinaryNode *ConstantFold::NewBinaryNode(BinaryNode *old, Opcode op, PrimType primType,
                                        BaseNode *lhs, BaseNode *rhs) const {
  CHECK_NULL_FATAL(old);
  BinaryNode *result = nullptr;
  if (old->GetOpCode() == op && old->GetPrimType() == primType && old->Opnd(0) == lhs && old->Opnd(1) == rhs) {
    result = old;
  } else {
    result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(op, primType, lhs, rhs);
  }
  return result;
}

UnaryNode *ConstantFold::NewUnaryNode(UnaryNode *old, Opcode op, PrimType primType, BaseNode *expr) const {
  CHECK_NULL_FATAL(old);
  UnaryNode *result = nullptr;
  if (old->GetOpCode() == op && old->GetPrimType() == primType && old->Opnd(0) == expr) {
    result = old;
  } else {
    result = mirModule->CurFuncCodeMemPool()->New<UnaryNode>(op, primType, expr);
  }
  return result;
}

BaseNode *ConstantFold::PairToExpr(PrimType resultType, const std::pair<BaseNode*, int64> &pair) const {
  CHECK_NULL_FATAL(pair.first);
  BaseNode *result = pair.first;
  if (pair.second == 0) {
    return result;
  }
  if (pair.first->GetOpCode() == OP_neg && pair.second > 0) {
    // -a, 5 -> 5 - a
    ConstvalNode *val = mirModule->GetMIRBuilder()->CreateIntConst(pair.second, resultType);
    BaseNode *r = static_cast<UnaryNode*>(pair.first)->Opnd(0);
    result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_sub, resultType, val, r);
  } else {
    if (pair.second > 0 || pair.second == LLONG_MIN) {
      // +-a, 5 -> a + 5
      ConstvalNode *val = mirModule->GetMIRBuilder()->CreateIntConst(pair.second, resultType);
      result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_add, resultType, pair.first, val);
    } else {
      // +-a, -5 -> a + -5
      ConstvalNode *val = mirModule->GetMIRBuilder()->CreateIntConst(-pair.second, resultType);
      result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_sub, resultType, pair.first, val);
    }
  }
  return result;
}

std::pair<BaseNode*, int64> ConstantFold::FoldBase(BaseNode *node) const {
  return std::make_pair(node, 0);
}

StmtNode *ConstantFold::Simplify(StmtNode *node) {
  CHECK_NULL_FATAL(node);
  switch (node->GetOpCode()) {
    case OP_dassign:
    case OP_maydassign:
      return SimplifyDassign(static_cast<DassignNode*>(node));
    case OP_iassign:
      return SimplifyIassign(static_cast<IassignNode*>(node));
    case OP_block:
      return SimplifyBlock(static_cast<BlockNode*>(node));
    case OP_if:
      return SimplifyIf(static_cast<IfStmtNode*>(node));
    case OP_dowhile:
    case OP_while:
      return SimplifyWhile(static_cast<WhileStmtNode*>(node));
    case OP_switch:
      return SimplifySwitch(static_cast<SwitchNode*>(node));
    case OP_eval:
    case OP_throw:
    case OP_free:
    case OP_decref:
    case OP_incref:
    case OP_decrefreset:
    case OP_regassign:
    case OP_assertnonnull:
    case OP_igoto:
      return SimplifyUnary(static_cast<UnaryStmtNode*>(node));
    case OP_assertge:
    case OP_assertlt:
      return SimplifyBinary(static_cast<BinaryStmtNode*>(node));
    case OP_brfalse:
    case OP_brtrue:
      return SimplifyCondGoto(static_cast<CondGotoNode*>(node));
    case OP_return:
    case OP_syncenter:
    case OP_syncexit:
    case OP_call:
    case OP_virtualcall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallwithtype:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned:
    case OP_xintrinsiccallassigned:
    case OP_callinstant:
    case OP_callinstantassigned:
    case OP_virtualcallinstant:
    case OP_virtualcallinstantassigned:
    case OP_superclasscallinstant:
    case OP_superclasscallinstantassigned:
    case OP_interfacecallinstant:
    case OP_interfacecallinstantassigned:
      return SimplifyNary(static_cast<NaryStmtNode*>(node));
    case OP_icall:
    case OP_icallassigned:
      return SimplifyIcall(static_cast<IcallNode*>(node));
    case OP_asm:
      return SimplifyAsm(static_cast<AsmNode*>(node));
    default:
      return node;
  }
}

std::pair<BaseNode*, int64> ConstantFold::DispatchFold(BaseNode *node) {
  CHECK_NULL_FATAL(node);
  switch (node->GetOpCode()) {
    case OP_sizeoftype:
      return FoldSizeoftype(static_cast<SizeoftypeNode*>(node));
    case OP_abs:
    case OP_bnot:
    case OP_lnot:
    case OP_neg:
    case OP_recip:
    case OP_sqrt:
      return FoldUnary(static_cast<UnaryNode*>(node));
    case OP_ceil:
    case OP_floor:
    case OP_round:
    case OP_trunc:
    case OP_cvt:
      return FoldTypeCvt(static_cast<TypeCvtNode*>(node));
    case OP_sext:
    case OP_zext:
    case OP_extractbits:
      return FoldExtractbits(static_cast<ExtractbitsNode*>(node));
    case OP_iaddrof:
    case OP_iread:
      return FoldIread(static_cast<IreadNode*>(node));
    case OP_add:
    case OP_ashr:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_cior:
    case OP_div:
    case OP_land:
    case OP_lior:
    case OP_lshr:
    case OP_max:
    case OP_min:
    case OP_mul:
    case OP_rem:
    case OP_shl:
    case OP_sub:
      return FoldBinary(static_cast<BinaryNode*>(node));
    case OP_eq:
    case OP_ne:
    case OP_ge:
    case OP_gt:
    case OP_le:
    case OP_lt:
    case OP_cmp:
      return FoldCompare(static_cast<CompareNode*>(node));
    case OP_depositbits:
      return FoldDepositbits(static_cast<DepositbitsNode*>(node));
    case OP_select:
      return FoldTernary(static_cast<TernaryNode*>(node));
    case OP_array:
      return FoldArray(static_cast<ArrayNode*>(node));
    case OP_retype:
      return FoldRetype(static_cast<RetypeNode*>(node));
    case OP_gcmallocjarray:
    case OP_gcpermallocjarray:
      return FoldGcmallocjarray(static_cast<JarrayMallocNode*>(node));
    default:
      return FoldBase(static_cast<BaseNode*>(node));
  }
}

BaseNode *ConstantFold::Negate(BaseNode *node) const {
  CHECK_NULL_FATAL(node);
  return mirModule->CurFuncCodeMemPool()->New<UnaryNode>(OP_neg, PrimType(node->GetPrimType()), node);
}

BaseNode *ConstantFold::Negate(UnaryNode *node) const {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  if (node->GetOpCode() == OP_neg) {
    result = static_cast<BaseNode*>(node->Opnd(0));
  } else {
    BaseNode *n = static_cast<BaseNode*>(node);
    result = NewUnaryNode(node, OP_neg, node->GetPrimType(), n);
  }
  return result;
}

BaseNode *ConstantFold::Negate(const ConstvalNode *node) const {
  CHECK_NULL_FATAL(node);
  ConstvalNode *copy = node->CloneTree(mirModule->GetCurFuncCodeMPAllocator());
  CHECK_NULL_FATAL(copy);
  copy->GetConstVal()->Neg();
  return copy;
}

BaseNode *ConstantFold::NegateTree(BaseNode *node) const {
  CHECK_NULL_FATAL(node);
  if (node->IsUnaryNode()) {
    return Negate(static_cast<UnaryNode*>(node));
  } else if (node->GetOpCode() == OP_constval) {
    return Negate(static_cast<ConstvalNode*>(node));
  } else {
    return Negate(static_cast<BaseNode*>(node));
  }
}

MIRIntConst *ConstantFold::FoldIntConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                          const MIRIntConst &intConst0,
                                                          const MIRIntConst &intConst1) const {
  int64 result = 0;
  bool greater = (intConst0.GetValue() > intConst1.GetValue());
  bool equal = (intConst0.GetValue() == intConst1.GetValue());
  bool less = (intConst0.GetValue() < intConst1.GetValue());
  constexpr uint32 eightBytes = 8;
  bool use64 = GetPrimTypeSize(opndType) == eightBytes;
  switch (opcode) {
    case OP_eq: {
      if (use64) {
        result = equal;
      } else {
        result = static_cast<int32>(intConst0.GetValue()) == static_cast<int32>(intConst1.GetValue());
      }
      break;
    }
    case OP_ge: {
      if (IsUnsignedInteger(opndType)) {
        if (use64) {
          result = static_cast<uint64>(intConst0.GetValue()) >= static_cast<uint64>(intConst1.GetValue());
        } else {
          result = static_cast<uint32>(intConst0.GetValue()) >= static_cast<uint32>(intConst1.GetValue());
        }
      } else {
        if (use64) {
          result = (greater || equal);
        } else {
          result = static_cast<int32>(intConst0.GetValue()) >= static_cast<int32>(intConst1.GetValue());
        }
      }
      break;
    }
    case OP_gt: {
      if (IsUnsignedInteger(opndType)) {
        if (use64) {
          result = static_cast<uint64>(intConst0.GetValue()) > static_cast<uint64>(intConst1.GetValue());
        } else {
          result = static_cast<uint32>(intConst0.GetValue()) > static_cast<uint32>(intConst1.GetValue());
        }
      } else {
        if (use64) {
          result = greater;
        } else {
          result = static_cast<int32>(intConst0.GetValue()) > static_cast<int32>(intConst1.GetValue());
        }
      }
      break;
    }
    case OP_le: {
      if (IsUnsignedInteger(opndType)) {
        if (use64) {
          result = static_cast<uint64>(intConst0.GetValue()) <= static_cast<uint64>(intConst1.GetValue());
        } else {
          result = static_cast<uint32>(intConst0.GetValue()) <= static_cast<uint32>(intConst1.GetValue());
        }
      } else {
        if (use64) {
          result = (less || equal);
        } else {
          result = static_cast<int32>(intConst0.GetValue()) <= static_cast<int32>(intConst1.GetValue());
        }
      }
      break;
    }
    case OP_lt: {
      if (IsUnsignedInteger(opndType)) {
        if (use64) {
          result = static_cast<uint64>(intConst0.GetValue()) < static_cast<uint64>(intConst1.GetValue());
        } else {
          result = static_cast<uint32>(intConst0.GetValue()) < static_cast<uint32>(intConst1.GetValue());
        }
      } else {
        if (use64) {
          result = less;
        } else {
          result = static_cast<int32>(intConst0.GetValue()) < static_cast<int32>(intConst1.GetValue());
        }
      }
      break;
    }
    case OP_ne: {
      if (use64) {
        result = !equal;
      } else {
        result = static_cast<int32>(intConst0.GetValue()) != static_cast<int32>(intConst1.GetValue());
      }
      break;
    }
    case OP_cmp: {
      if (IsUnsignedInteger(opndType)) {
        if (use64) {
          if (static_cast<uint64>(intConst0.GetValue()) > static_cast<uint64>(intConst1.GetValue())) {
            result = kGreater;
          }
          if (static_cast<uint64>(intConst0.GetValue()) == static_cast<uint64>(intConst1.GetValue())) {
            result = kEqual;
          }
          if (static_cast<uint64>(intConst0.GetValue()) < static_cast<uint64>(intConst1.GetValue())) {
            result = kLess;
          }
        } else {
          if (static_cast<uint32>(intConst0.GetValue()) > static_cast<uint32>(intConst1.GetValue())) {
            result = kGreater;
          }
          if (static_cast<uint32>(intConst0.GetValue()) == static_cast<uint32>(intConst1.GetValue())) {
            result = kEqual;
          }
          if (static_cast<uint32>(intConst0.GetValue()) < static_cast<uint32>(intConst1.GetValue())) {
            result = kLess;
          }
        }
      } else {
        if (use64) {
          if (intConst0.GetValue() > intConst1.GetValue()) {
            result = kGreater;
          }
          if (intConst0.GetValue() == intConst1.GetValue()) {
            result = kEqual;
          }
          if (intConst0.GetValue() < intConst1.GetValue()) {
            result = kLess;
          }
        } else {
          if (static_cast<int32>(intConst0.GetValue()) > static_cast<int32>(intConst1.GetValue())) {
            result = kGreater;
          }
          if (static_cast<int32>(intConst0.GetValue()) == static_cast<int32>(intConst1.GetValue())) {
            result = kEqual;
          }
          if (static_cast<int32>(intConst0.GetValue()) < static_cast<int32>(intConst1.GetValue())) {
            result = kLess;
          }
        }
      }
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for FoldIntConstComparison");
      break;
  }
  // determine the type
  MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
  // form the constant
  MIRIntConst *constValue = nullptr;
  if (type.GetPrimType() == PTY_dyni32) {
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, type);
    constValue->SetValue(kJsTypeNumberInHigh32Bit | (static_cast<uint64>(result)));
  } else {
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(result, type);
  }
  return constValue;
}

ConstvalNode *ConstantFold::FoldIntConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                                   const ConstvalNode &const0, const ConstvalNode &const1) const {
  const MIRIntConst *intConst0 = safe_cast<MIRIntConst>(const0.GetConstVal());
  const MIRIntConst *intConst1 = safe_cast<MIRIntConst>(const1.GetConstVal());
  CHECK_NULL_FATAL(intConst0);
  CHECK_NULL_FATAL(intConst1);
  MIRIntConst *constValue = FoldIntConstComparisonMIRConst(opcode, resultType, opndType, *intConst0, *intConst1);
  // form the ConstvalNode
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(resultType);
  resultConst->SetConstVal(constValue);
  return resultConst;
}

MIRConst *ConstantFold::FoldIntConstBinaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst *intConst0,
                                                   const MIRIntConst *intConst1) const {
  int64 intValueOfConst0 = intConst0->GetValue();
  int64 intValueOfConst1 = intConst1->GetValue();
  uint64 result64 = 0;
  uint32 result32 = 0;
  bool useResult64 = (GetPrimTypeSize(resultType) == kByteSizeOfBit64);
  switch (opcode) {
    case OP_add: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) + static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) + static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_sub: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) - static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) - static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_mul: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) * static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) * static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_div: {
      if (IsUnsignedInteger(resultType)) {
        if (useResult64) {
          result64 = static_cast<uint64>(intValueOfConst0) / static_cast<uint64>(intValueOfConst1);
        } else {
          result32 = static_cast<uint32>(intValueOfConst0) / static_cast<uint32>(intValueOfConst1);
        }
      } else {
        if (useResult64) {
          result64 = static_cast<uint64>(intValueOfConst0 / intValueOfConst1);
        } else {
          result32 = static_cast<uint32>(static_cast<int32>(intValueOfConst0) / static_cast<int32>(intValueOfConst1));
        }
      }
      break;
    }
    case OP_rem: {
      if (IsUnsignedInteger(resultType)) {
        if (useResult64) {
          result64 = static_cast<uint64>(intValueOfConst0) % static_cast<uint64>(intValueOfConst1);
        } else {
          result32 = static_cast<uint32>(intValueOfConst0) % static_cast<uint32>(intValueOfConst1);
        }
      } else {
        if (useResult64) {
          result64 = static_cast<uint64>(intValueOfConst0 % intValueOfConst1);
        } else {
          result32 = static_cast<int32>(intValueOfConst0) % static_cast<int32>(intValueOfConst1);
        }
      }
      break;
    }
    case OP_ashr: {
      if (useResult64) {
        result64 = intValueOfConst0 >> static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<int32>(intValueOfConst0) >> static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_lshr: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) >> static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) >> static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_shl: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) << static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) << static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_max: {
      if (IsUnsignedInteger(resultType)) {
        if (useResult64) {
          result64 = (static_cast<uint64>(intValueOfConst0) >= static_cast<uint64>(intValueOfConst1)) ?
                     static_cast<uint64>(intValueOfConst0) : static_cast<uint64>(intValueOfConst1);
        } else {
          result32 = (static_cast<uint32>(intValueOfConst0) >= static_cast<uint32>(intValueOfConst1)) ?
                     static_cast<uint32>(intValueOfConst0) : static_cast<uint32>(intValueOfConst1);
        }
      } else {
        if (useResult64) {
          result64 = (intValueOfConst0 >= intValueOfConst1) ?
              static_cast<uint64>(intValueOfConst0) : static_cast<uint64>(intValueOfConst1);
        } else {
          result32 = (static_cast<int32>(intValueOfConst0) >= static_cast<int32>(intValueOfConst1)) ?
                     static_cast<uint32>(intValueOfConst0) : static_cast<uint32>(intValueOfConst1);
        }
      }
      break;
    }
    case OP_min: {
      if (IsUnsignedInteger(resultType)) {
        if (useResult64) {
          result64 = (static_cast<uint64>(intValueOfConst0) <= static_cast<uint64>(intValueOfConst1)) ?
                     static_cast<uint64>(intValueOfConst0) : static_cast<uint64>(intValueOfConst1);
        } else {
          result32 = (static_cast<uint32>(intValueOfConst0) <= static_cast<uint32>(intValueOfConst1)) ?
                     static_cast<uint32>(intValueOfConst0) : static_cast<uint32>(intValueOfConst1);
        }
      } else {
        if (useResult64) {
          result64 = (intValueOfConst0 <= intValueOfConst1) ?
                     static_cast<uint64>(intValueOfConst0) : static_cast<uint64>(intValueOfConst1);
        } else {
          result32 = (static_cast<int32>(intValueOfConst0) <= static_cast<int32>(intValueOfConst1)) ?
                     static_cast<uint32>(intValueOfConst0) : static_cast<uint32>(intValueOfConst1);
        }
      }
      break;
    }
    case OP_band: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) & static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) & static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_bior: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) | static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) | static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_bxor: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0) ^ static_cast<uint64>(intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0) ^ static_cast<uint32>(intValueOfConst1);
      }
      break;
    }
    case OP_cand:
    case OP_land: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0 && intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0 && intValueOfConst1);
      }
      break;
    }
    case OP_cior:
    case OP_lior: {
      if (useResult64) {
        result64 = static_cast<uint64>(intValueOfConst0 || intValueOfConst1);
      } else {
        result32 = static_cast<uint32>(intValueOfConst0 || intValueOfConst1);
      }
      break;
    }
    case OP_depositbits: {
      // handled in FoldDepositbits
      ASSERT(false, "Unexpected opcode in FoldIntConstBinary");
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for FoldIntConstBinary");
      break;
  }
  // determine the type
  MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
  // form the constant
  MIRIntConst *constValue = nullptr;
  if (type.GetPrimType() == PTY_dyni32) {
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, type);
    constValue->SetValue(kJsTypeNumberInHigh32Bit | (static_cast<uint64>(result32)));
  } else if (useResult64) {
    constValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(result64), type);
  } else {
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(result32, type);
  }
  return constValue;
}

ConstvalNode *ConstantFold::FoldIntConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                               const ConstvalNode &const1) const {
  const MIRIntConst *intConst0 = safe_cast<MIRIntConst>(const0.GetConstVal());
  const MIRIntConst *intConst1 = safe_cast<MIRIntConst>(const1.GetConstVal());
  CHECK_NULL_FATAL(intConst0);
  CHECK_NULL_FATAL(intConst1);
  MIRConst *constValue = FoldIntConstBinaryMIRConst(opcode, resultType, intConst0, intConst1);
  // form the ConstvalNode
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(resultType);
  resultConst->SetConstVal(constValue);
  return resultConst;
}

ConstvalNode *ConstantFold::FoldFPConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                              const ConstvalNode &const1) const {
  ASSERT(const0.GetPrimType() == const1.GetPrimType(), "The types of the operands must match");
  const MIRDoubleConst *doubleConst0 = nullptr;
  const MIRDoubleConst *doubleConst1 = nullptr;
  const MIRFloatConst *floatConst0 = nullptr;
  const MIRFloatConst *floatConst1 = nullptr;
  bool useDouble = (const0.GetPrimType() == PTY_f64);
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(resultType);
  if (useDouble) {
    doubleConst0 = safe_cast<MIRDoubleConst>(const0.GetConstVal());
    doubleConst1 = safe_cast<MIRDoubleConst>(const1.GetConstVal());
    CHECK_NULL_FATAL(doubleConst0);
    CHECK_NULL_FATAL(doubleConst1);
  } else {
    floatConst0 = safe_cast<MIRFloatConst>(const0.GetConstVal());
    floatConst1 = safe_cast<MIRFloatConst>(const1.GetConstVal());
    CHECK_NULL_FATAL(floatConst0);
    CHECK_NULL_FATAL(floatConst1);
  }
  float constValueFloat = 0.0;
  double constValueDouble = 0.0;
  switch (opcode) {
    case OP_add: {
      if (useDouble) {
        constValueDouble = doubleConst0->GetValue() + doubleConst1->GetValue();
      } else {
        constValueFloat = floatConst0->GetValue() + floatConst1->GetValue();
      }
      break;
    }
    case OP_sub: {
      if (useDouble) {
        constValueDouble = doubleConst0->GetValue() - doubleConst1->GetValue();
      } else {
        constValueFloat = floatConst0->GetValue() - floatConst1->GetValue();
      }
      break;
    }
    case OP_mul: {
      if (useDouble) {
        constValueDouble = doubleConst0->GetValue() * doubleConst1->GetValue();
      } else {
        constValueFloat = floatConst0->GetValue() * floatConst1->GetValue();
      }
      break;
    }
    case OP_div: {
      // for floats div by 0 is well defined
      if (useDouble) {
        constValueDouble = doubleConst0->GetValue() / doubleConst1->GetValue();
      } else {
        constValueFloat = floatConst0->GetValue() / floatConst1->GetValue();
      }
      break;
    }
    case OP_max: {
      if (useDouble) {
        constValueDouble = (doubleConst0->GetValue() >= doubleConst1->GetValue()) ? doubleConst0->GetValue()
                                                                                  : doubleConst1->GetValue();
      } else {
        constValueFloat = (floatConst0->GetValue() >= floatConst1->GetValue()) ? floatConst0->GetValue()
                                                                               : floatConst1->GetValue();
      }
      break;
    }
    case OP_min: {
      if (useDouble) {
        constValueDouble = (doubleConst0->GetValue() <= doubleConst1->GetValue()) ? doubleConst0->GetValue()
                                                                                  : doubleConst1->GetValue();
      } else {
        constValueFloat = (floatConst0->GetValue() <= floatConst1->GetValue()) ? floatConst0->GetValue()
                                                                               : floatConst1->GetValue();
      }
      break;
    }
    case OP_rem:
    case OP_ashr:
    case OP_lshr:
    case OP_shl:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_land:
    case OP_cior:
    case OP_lior:
    case OP_depositbits: {
      ASSERT(false, "Unexpected opcode in FoldFPConstBinary");
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for FoldFPConstBinary");
      break;
  }
  if (resultType == PTY_f64) {
    resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(constValueDouble));
  } else {
    resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateFloatConst(constValueFloat));
  }
  return resultConst;
}


bool ConstantFold::ConstValueEqual(int64 leftValue, int64 rightValue) const {
  return (leftValue == rightValue);
}

bool ConstantFold::ConstValueEqual(float leftValue, float rightValue) const {
  return fabs(leftValue - rightValue) <= FLT_MIN;
}

bool ConstantFold::ConstValueEqual(double leftValue, double rightValue) const {
  return fabs(leftValue - rightValue) <= DBL_MIN;
}

template<typename T>
bool ConstantFold::FullyEqual(T leftValue, T rightValue) const {
  if (isinf(leftValue) && isinf(rightValue)) {
    // (inf == inf), add the judgement here in case of the subtraction between float type inf
    return true;
  } else {
    return ConstValueEqual(leftValue, rightValue);
  }
}

template<typename T>
int64 ConstantFold::ComparisonResult(Opcode op, T *leftConst, T *rightConst) const {
  typename T::value_type leftValue = leftConst->GetValue();
  typename T::value_type rightValue = rightConst->GetValue();
  int64 result = 0;;
  switch (op) {
    case OP_eq: {
      result = FullyEqual(leftValue, rightValue);
      break;
    }
    case OP_ge: {
      result = (leftValue > rightValue) || FullyEqual(leftValue, rightValue);
      break;
    }
    case OP_gt: {
      result = (leftValue > rightValue);
      break;
    }
    case OP_le: {
      result = (leftValue < rightValue) || FullyEqual(leftValue, rightValue);
      break;
    }
    case OP_lt: {
      result = (leftValue < rightValue);
      break;
    }
    case OP_ne: {
      result = !FullyEqual(leftValue, rightValue);
      break;
    }
    case OP_cmpl:
    case OP_cmpg: {
      if (std::isnan(leftValue) || std::isnan(rightValue)) {
        result = (op == OP_cmpg) ? kGreater : kLess;
        break;
      }
    }
    [[clang::fallthrough]];
    case OP_cmp: {
      if (leftValue > rightValue) {
        result = kGreater;
      } else if (FullyEqual(leftValue, rightValue)) {
        result = kEqual;
      } else {
        result = kLess;
      }
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for Comparison");
      break;
  }
  return result;
}

MIRIntConst *ConstantFold::FoldFPConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                         const MIRConst &leftConst, const MIRConst &rightConst) const {
  int64 result = 0;
  bool useDouble = (opndType == PTY_f64);
  if (useDouble) {
    result = ComparisonResult(opcode, safe_cast<MIRDoubleConst>(&leftConst), safe_cast<MIRDoubleConst>(&rightConst));
  } else {
    result = ComparisonResult(opcode, safe_cast<MIRFloatConst>(&leftConst), safe_cast<MIRFloatConst>(&rightConst));
  }
  MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
  MIRIntConst *resultConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(result, type);
  return resultConst;
}

ConstvalNode *ConstantFold::FoldFPConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                                  const ConstvalNode &const0, const ConstvalNode &const1) const {
  ASSERT(const0.GetPrimType() == const1.GetPrimType(), "The types of the operands must match");
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(resultType);
  resultConst->SetConstVal(FoldFPConstComparisonMIRConst(opcode, resultType, opndType, *const0.GetConstVal(),
                                                         *const1.GetConstVal()));
  return resultConst;
}

MIRConst *ConstantFold::FoldConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                    const MIRConst &const0, const MIRConst &const1) {
  MIRConst *returnValue = nullptr;
  if (IsPrimitiveInteger(opndType) || IsPrimitiveDynInteger(opndType)) {
    const auto *intConst0 = safe_cast<MIRIntConst>(&const0);
    const auto *intConst1 = safe_cast<MIRIntConst>(&const1);
    ASSERT_NOT_NULL(intConst0);
    ASSERT_NOT_NULL(intConst1);
    returnValue = FoldIntConstComparisonMIRConst(opcode, resultType, opndType, *intConst0, *intConst1);
  } else if (opndType == PTY_f32 || opndType == PTY_f64) {
    returnValue = FoldFPConstComparisonMIRConst(opcode, resultType, opndType, const0, const1);
  } else {
    ASSERT(false, "Unhandled case for FoldConstComparisonMIRConst");
  }
  return returnValue;
}

ConstvalNode *ConstantFold::FoldConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                                const ConstvalNode &const0, const ConstvalNode &const1) const {
  ConstvalNode *returnValue = nullptr;
  if (IsPrimitiveInteger(opndType) || IsPrimitiveDynInteger(opndType)) {
    returnValue = FoldIntConstComparison(opcode, resultType, opndType, const0, const1);
  } else if (opndType == PTY_f32 || opndType == PTY_f64) {
    returnValue = FoldFPConstComparison(opcode, resultType, opndType, const0, const1);
  } else {
    ASSERT(false, "Unhandled case for FoldConstComparison");
  }
  return returnValue;
}

CompareNode *ConstantFold::FoldConstComparisonReverse(Opcode opcode, PrimType resultType, PrimType opndType,
                                                      BaseNode &l, BaseNode &r) {
  CompareNode *result = nullptr;
  Opcode op = opcode;
  switch (opcode) {
    case OP_gt: {
      op = OP_lt;
      break;
    }
    case OP_lt: {
      op = OP_gt;
      break;
    }
    case OP_ge: {
      op = OP_le;
      break;
    }
    case OP_le: {
      op = OP_ge;
      break;
    }
    case OP_eq: {
      break;
    }
    case OP_ne: {
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for FoldConstComparisonReverse");
      break;
  }

  result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(
      Opcode(op), PrimType(resultType), PrimType(opndType), &r, &l);
  return result;
}

ConstvalNode *ConstantFold::FoldConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                            const ConstvalNode &const1) const {
  ConstvalNode *returnValue = nullptr;
  if (IsPrimitiveInteger(resultType) || IsPrimitiveDynInteger(resultType)) {
    returnValue = FoldIntConstBinary(opcode, resultType, const0, const1);
  } else if (resultType == PTY_f32 || resultType == PTY_f64) {
    returnValue = FoldFPConstBinary(opcode, resultType, const0, const1);
  } else {
    ASSERT(false, "Unhandled case for FoldConstBinary");
  }
  return returnValue;
}

ConstvalNode *ConstantFold::FoldIntConstUnary(Opcode opcode, PrimType resultType, const ConstvalNode *constNode) const {
  CHECK_NULL_FATAL(constNode);
  const MIRIntConst *cst = safe_cast<MIRIntConst>(constNode->GetConstVal());
  ASSERT_NOT_NULL(cst);
  uint32 result32 = 0;
  uint64 result64 = 0;
  bool useResult64 = (GetPrimTypeSize(resultType) == kByteSizeOfBit64);
  switch (opcode) {
    case OP_abs: {
      if (IsUnsignedInteger(resultType)) {
        if (useResult64) {
          result64 = static_cast<uint64>(cst->GetValue());
        } else {
          result32 = static_cast<uint32>(cst->GetValue());
        }
      } else {
        if (useResult64) {
          result64 = (cst->GetValue() >= 0) ? cst->GetValue() : -cst->GetValue();
        } else {
          result32 = (static_cast<int32>(cst->GetValue()) >= 0) ?
                     cst->GetValue() : -static_cast<int32>(cst->GetValue());
        }
      }
      break;
    }
    case OP_bnot: {
      if (useResult64) {
        result64 = ~static_cast<uint64>(cst->GetValue());
      } else {
        result32 = ~static_cast<uint32>(cst->GetValue());
      }
      break;
    }
    case OP_lnot: {
      if (useResult64) {
        result64 = cst->GetValue() == 0;
      } else {
        result32 = static_cast<uint32>(cst->GetValue()) == 0;
      }
      break;
    }
    case OP_neg: {
      if (useResult64) {
        result64 = -cst->GetValue();
      } else {
        result32 = static_cast<uint32>(-(cst->GetValue()));
      }
      break;
    }
    case OP_sext:         // handled in FoldExtractbits
    case OP_zext:         // handled in FoldExtractbits
    case OP_extractbits:  // handled in FoldExtractbits
    case OP_recip:
    case OP_sqrt: {
      ASSERT(false, "Unexpected opcode in FoldIntConstUnary");
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for FoldIntConstUnary");
      break;
  }
  // determine the type
  MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
  // form the constant
  MIRIntConst *constValue = nullptr;
  if (type.GetPrimType() == PTY_dyni32) {
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, type);
    constValue->SetValue(kJsTypeNumberInHigh32Bit | (static_cast<uint64>(result32)));
  } else if (useResult64) {
    constValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(result64), type);
  } else {
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(result32, type);
  }
  // form the ConstvalNode
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(type.GetPrimType());
  resultConst->SetConstVal(constValue);
  return resultConst;
}

template <typename T>
ConstvalNode *ConstantFold::FoldFPConstUnary(Opcode opcode, PrimType resultType, ConstvalNode *constNode) const {
  CHECK_NULL_FATAL(constNode);
  typename T::value_type constValue = 0.0;
  T *fpCst = static_cast<T*>(constNode->GetConstVal());
  switch (opcode) {
    case OP_recip:
    case OP_neg:
    case OP_abs: {
      if (OP_recip == opcode) {
        constValue = typename T::value_type(1.0 / fpCst->GetValue());
      } else if (OP_neg == opcode) {
        constValue = typename T::value_type(-fpCst->GetValue());
      } else if (OP_abs == opcode) {
        constValue = typename T::value_type(fabs(fpCst->GetValue()));
      }
      break;
    }
    case OP_sqrt: {
      constValue = typename T::value_type(sqrt(fpCst->GetValue()));
      break;
    }
    case OP_bnot:
    case OP_lnot:
    case OP_sext:
    case OP_zext:
    case OP_extractbits: {
      ASSERT(false, "Unexpected opcode in FoldFPConstUnary");
      break;
    }
    default:
      ASSERT(false, "Unknown opcode for FoldFPConstUnary");
      break;
  }
  auto *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(resultType);
  if (resultType == PTY_f32) {
    resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateFloatConst(constValue));
  } else {
    resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(constValue));
  }
  return resultConst;
}

ConstvalNode *ConstantFold::FoldConstUnary(Opcode opcode, PrimType resultType, ConstvalNode *constNode) const {
  ConstvalNode *returnValue = nullptr;
  if (IsPrimitiveInteger(resultType) || IsPrimitiveDynInteger(resultType)) {
    returnValue = FoldIntConstUnary(opcode, resultType, constNode);
  } else if (resultType == PTY_f32) {
    returnValue = FoldFPConstUnary<MIRFloatConst>(opcode, resultType, constNode);
  } else if (resultType == PTY_f64) {
    returnValue = FoldFPConstUnary<MIRDoubleConst>(opcode, resultType, constNode);
  } else if (PTY_f128 == resultType) {
  } else {
    ASSERT(false, "Unhandled case for FoldConstUnary");
  }
  return returnValue;
}

std::pair<BaseNode*, int64> ConstantFold::FoldSizeoftype(SizeoftypeNode *node) const {
  CHECK_NULL_FATAL(node);
  BaseNode *result = node;
  MIRType *argType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(node->GetTyIdx());
  if (argType->GetKind() == kTypeScalar) {
    MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(node->GetPrimType());
    uint32 size = GetPrimTypeSize(argType->GetPrimType());
    ConstvalNode *constValueNode = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    constValueNode->SetPrimType(node->GetPrimType());
    constValueNode->SetConstVal(GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<int64>(size), resultType));
    result = constValueNode;
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldRetype(RetypeNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = node;
  std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(0));
  if (node->Opnd(0) != p.first) {
    RetypeNode *newRetNode = node->CloneTree(mirModule->GetCurFuncCodeMPAllocator());
    CHECK_FATAL(newRetNode != nullptr, "newRetNode is null in ConstantFold::FoldRetype");
    newRetNode->SetOpnd(PairToExpr(node->Opnd(0)->GetPrimType(), p), 0);
    result = newRetNode;
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldGcmallocjarray(JarrayMallocNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = node;
  std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(0));
  if (node->Opnd(0) != p.first) {
    JarrayMallocNode *newRetNode = node->CloneTree(mirModule->GetCurFuncCodeMPAllocator());
    CHECK_FATAL(newRetNode != nullptr, "newRetNode is null in ConstantFold::FoldGcmallocjarray");
    newRetNode->SetOpnd(PairToExpr(node->Opnd(0)->GetPrimType(), p), 0);
    result = newRetNode;
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldUnary(UnaryNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  int64 sum = 0;
  std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(0));
  ConstvalNode *cst = safe_cast<ConstvalNode>(p.first);
  if (cst != nullptr) {
    result = FoldConstUnary(node->GetOpCode(), node->GetPrimType(), cst);
    sum = 0;
  } else {
    bool isInt = IsPrimitiveInteger(node->GetPrimType());
    // The neg node will be recreated regardless of whether the folding is successful or not. And the neg node's
    // primType will be set to opnd type. There will be problems in some cases. For example:
    // before cf:
    //   neg i32 (eq u1 f32 (dread f32 %f_4_2, constval f32 0f))
    // after cf:
    //   neg u1 (eq u1 f32 (dread f32 %f_4_2, constval f32 0f))  # wrong!
    // As a workaround, we exclude u1 opnd type
    if (isInt && node->GetOpCode() == OP_neg && p.first->GetPrimType() != PTY_u1) {
      result = NegateTree(p.first);
      if (result->GetOpCode() == OP_neg && result->GetPrimType() == node->GetPrimType() &&
          static_cast<UnaryNode*>(result)->Opnd(0) == node->Opnd(0)) {
        // NegateTree returned an UnaryNode quivalent to `n`, so keep the
        // original UnaryNode to preserve identity
        result = node;
      }
      sum = -p.second;
    } else {
      result = NewUnaryNode(node, node->GetOpCode(), node->GetPrimType(), PairToExpr(node->Opnd(0)->GetPrimType(), p));
      sum = 0;
    }
  }
  return std::make_pair(result, sum);
}

static bool FloatToIntOverflow(float fval, PrimType totype) {
  static const float safeFloatMaxToInt32 = 2147483520.0f; // 2^31 - 128
  static const float safeFloatMinToInt32 = -2147483520.0f;
  static const float safeFloatMaxToInt64 = 9223372036854775680.0f; // 2^63 - 128
  static const float safeFloatMinToInt64 = -9223372036854775680.0f;
  if (!std::isfinite(fval)) {
    return true;
  }
  if (totype == PTY_i64 || totype == PTY_u64) {
    if (fval < safeFloatMinToInt64 || fval > safeFloatMaxToInt64) {
      return true;
    }
  } else {
    if (fval < safeFloatMinToInt32 || fval > safeFloatMaxToInt32) {
      return true;
    }
  }
  return false;
}

static bool DoubleToIntOverflow(double dval, PrimType totype) {
  static const double safeDoubleMaxToInt32 = 2147482624.0; // 2^31 - 1024
  static const double safeDoubleMinToInt32 = -2147482624.0;
  static const double safeDoubleMaxToInt64 = 9223372036854774784.0; // 2^63 - 1024
  static const double safeDoubleMinToInt64 = -9223372036854774784.0;
  if (!std::isfinite(dval)) {
    return true;
  }
  if (totype == PTY_i64 || totype == PTY_u64) {
    if (dval < safeDoubleMinToInt64 || dval > safeDoubleMaxToInt64) {
      return true;
    }
  } else {
    if (dval < safeDoubleMinToInt32 || dval > safeDoubleMaxToInt32) {
      return true;
    }
  }
  return false;
}

ConstvalNode *ConstantFold::FoldCeil(const ConstvalNode &cst, PrimType fromType, PrimType toType) const {
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(toType);
  MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
  if (fromType == PTY_f32) {
    const MIRFloatConst *constValue = safe_cast<MIRFloatConst>(cst.GetConstVal());
    ASSERT_NOT_NULL(constValue);
    float floatValue = ceil(constValue->GetValue());
    if (FloatToIntOverflow(floatValue, toType)) {
      return nullptr;
    }
    resultConst->SetConstVal(GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<int64>(floatValue), resultType));
  } else {
    const MIRDoubleConst *constValue = safe_cast<MIRDoubleConst>(cst.GetConstVal());
    ASSERT_NOT_NULL(constValue);
    double doubleValue = ceil(constValue->GetValue());
    if (DoubleToIntOverflow(doubleValue, toType)) {
      return nullptr;
    }
    resultConst->SetConstVal(GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<int64>(doubleValue), resultType));
  }
  return resultConst;
}

template <class T>
T ConstantFold::CalIntValueFromFloatValue(T value, const MIRType &resultType) const {
  ASSERT(kByteSizeOfBit64 >= resultType.GetSize(), "unsurpported type");
  size_t shiftNum = (kByteSizeOfBit64 - resultType.GetSize()) * kBitSizePerByte;
  bool isSigned = IsSignedInteger(resultType.GetPrimType());
  int64 max = LONG_LONG_MAX >> shiftNum;
  uint64 umax = ULONG_LONG_MAX >> shiftNum;
  int64 min = isSigned ? (LONG_LONG_MIN >> shiftNum) : 0;
  if (isSigned && (value > max)) {
    return max;
  } else if (!isSigned && (value > umax)) {
    return umax;
  } else if (value < min) {
    return min;
  }
  return value;
}

MIRConst *ConstantFold::FoldFloorMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType, bool isFloor) const {
  MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
  if (fromType == PTY_f32) {
    const auto &constValue = static_cast<const MIRFloatConst&>(cst);
    float floatValue = constValue.GetValue();
    if (isFloor) {
      floatValue = floor(constValue.GetValue());
    }
    if (FloatToIntOverflow(floatValue, toType)) {
      return nullptr;
    }
    floatValue = CalIntValueFromFloatValue(floatValue, resultType);
    return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(floatValue), resultType);
  } else {
    const auto &constValue = static_cast<const MIRDoubleConst&>(cst);
    double doubleValue = constValue.GetValue();
    if (isFloor) {
      doubleValue = floor(constValue.GetValue());
    }
    if (DoubleToIntOverflow(doubleValue, toType)) {
      return nullptr;
    }
    doubleValue = CalIntValueFromFloatValue(doubleValue, resultType);
    return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(doubleValue), resultType);
  }
}

ConstvalNode *ConstantFold::FoldFloor(const ConstvalNode &cst, PrimType fromType, PrimType toType) const {
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(toType);
  resultConst->SetConstVal(FoldFloorMIRConst(*cst.GetConstVal(), fromType, toType));
  return resultConst;
}

MIRConst *ConstantFold::FoldRoundMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const {
  MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
  if (fromType == PTY_f32) {
    const auto &constValue = static_cast<const MIRFloatConst&>(cst);
    float floatValue = round(constValue.GetValue());
    if (FloatToIntOverflow(floatValue, toType)) {
      return nullptr;
    }
    return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(floatValue), resultType);
  } else if (fromType == PTY_f64) {
    const auto &constValue = static_cast<const MIRDoubleConst&>(cst);
    double doubleValue = round(constValue.GetValue());
    if (DoubleToIntOverflow(doubleValue, toType)) {
      return nullptr;
    }
    return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(doubleValue), resultType);
  } else if (toType == PTY_f32 && IsPrimitiveInteger(fromType)) {
    const auto &constValue = static_cast<const MIRIntConst&>(cst);
    if (IsSignedInteger(fromType)) {
      int64 fromValue = constValue.GetValue();
      float floatValue = round(static_cast<float>(fromValue));
      if (static_cast<int64>(floatValue) == fromValue) {
        return GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
      }
    } else {
      uint64 fromValue = static_cast<uint64>(constValue.GetValue());
      float floatValue = round(static_cast<float>(fromValue));
      if (static_cast<uint64>(floatValue) == fromValue) {
        return GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
      }
    }
  } else if (toType == PTY_f64 && IsPrimitiveInteger(fromType)) {
    const auto &constValue = static_cast<const MIRIntConst&>(cst);
    if (IsSignedInteger(fromType)) {
      int64 fromValue = constValue.GetValue();
      double doubleValue = round(static_cast<double>(fromValue));
      if (static_cast<int64>(doubleValue) == fromValue) {
        return GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
      }
    } else {
      uint64 fromValue = static_cast<uint64>(constValue.GetValue());
      double doubleValue = round(static_cast<double>(fromValue));
      if (static_cast<uint64>(doubleValue) == fromValue) {
        return GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
      }
    }
  }
  return nullptr;
}

ConstvalNode *ConstantFold::FoldRound(const ConstvalNode &cst, PrimType fromType, PrimType toType) const {
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(toType);
  resultConst->SetConstVal(FoldRoundMIRConst(*cst.GetConstVal(), fromType, toType));
  return resultConst;
}

ConstvalNode *ConstantFold::FoldTrunk(const ConstvalNode &cst, PrimType fromType, PrimType toType) const {
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  resultConst->SetPrimType(toType);
  MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
  if (fromType == PTY_f32) {
    const MIRFloatConst *constValue = safe_cast<MIRFloatConst>(cst.GetConstVal());
    CHECK_NULL_FATAL(constValue);
    float floatValue = trunc(constValue->GetValue());
    if (FloatToIntOverflow(floatValue, toType)) {
      return nullptr;
    }
    resultConst->SetConstVal(GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<int64>(floatValue), resultType));
  } else {
    const MIRDoubleConst *constValue = safe_cast<MIRDoubleConst>(cst.GetConstVal());
    CHECK_NULL_FATAL(constValue);
    double doubleValue = trunc(constValue->GetValue());
    if (DoubleToIntOverflow(doubleValue, toType)) {
      return nullptr;
    }
    resultConst->SetConstVal(GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<int64>(doubleValue), resultType));
  }
  return resultConst;
}

MIRConst *ConstantFold::FoldTypeCvtMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const {
  if (IsPrimitiveDynType(fromType) || IsPrimitiveDynType(toType)) {
    // do not fold
    return nullptr;
  }
  if (IsPrimitiveInteger(fromType) && IsPrimitiveInteger(toType)) {
    MIRConst *toConst = nullptr;
    uint32 fromSize = GetPrimTypeBitSize(fromType);
    uint32 toSize = GetPrimTypeBitSize(toType);
    // GetPrimTypeBitSize(PTY_u1) will return 8, which is not expected here.
    if (fromType == PTY_u1) {
      fromSize = 1;
    }
    if (toType == PTY_u1) {
      toSize = 1;
    }
    if (toSize > fromSize) {
      Opcode op = OP_zext;
      if (IsSignedInteger(fromType)) {
        op = OP_sext;
      }
      toConst = FoldSignExtendMIRConst(op, toType, fromSize, cst);
    } else {
      const MIRIntConst *constVal = safe_cast<MIRIntConst>(cst);
      ASSERT_NOT_NULL(constVal);
      MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(toType);
      toConst =
          GlobalTables::GetIntConstTable().GetOrCreateIntConst(constVal->GetValue(), type);
    }
    return toConst;
  }
  if (IsPrimitiveFloat(fromType) && IsPrimitiveFloat(toType)) {
    MIRConst *toConst = nullptr;
    if (GetPrimTypeBitSize(toType) < GetPrimTypeBitSize(fromType)) {
      ASSERT(GetPrimTypeBitSize(toType) == 32, "We suppot F32 and F64");
      const MIRDoubleConst *fromValue = safe_cast<MIRDoubleConst>(cst);
      ASSERT_NOT_NULL(fromValue);
      float floatValue = static_cast<float>(fromValue->GetValue());
      MIRFloatConst *toValue = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
      toConst = toValue;
    } else {
      ASSERT(GetPrimTypeBitSize(toType) == 64, "We suppot F32 and F64");
      const MIRFloatConst *fromValue = safe_cast<MIRFloatConst>(cst);
      ASSERT_NOT_NULL(fromValue);
      double doubleValue = static_cast<double>(fromValue->GetValue());
      MIRDoubleConst *toValue = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
      toConst = toValue;
    }
    return toConst;
  }
  if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
    return FoldFloorMIRConst(cst, fromType, toType, false);
  }
  if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType)) {
    return FoldRoundMIRConst(cst, fromType, toType);
  }
  CHECK_FATAL(false, "Unexpected case in ConstFoldTypeCvt");
  return nullptr;
}

ConstvalNode *ConstantFold::FoldTypeCvt(const ConstvalNode &cst, PrimType fromType, PrimType toType) const {
  MIRConst *toConstValue = FoldTypeCvtMIRConst(*cst.GetConstVal(), fromType, toType);
  if (toConstValue == nullptr) {
    return nullptr;
  }
  ConstvalNode *toConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  toConst->SetPrimType(toConstValue->GetType().GetPrimType());
  toConst->SetConstVal(toConstValue);
  return toConst;
}

std::pair<BaseNode*, int64> ConstantFold::FoldTypeCvt(TypeCvtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(0));
  ConstvalNode *cst = safe_cast<ConstvalNode>(p.first);
  if (cst != nullptr) {
    switch (node->GetOpCode()) {
      case OP_ceil: {
        result = FoldCeil(*cst, node->FromType(), node->GetPrimType());
        break;
      }
      case OP_cvt: {
        result = FoldTypeCvt(*cst, node->FromType(), node->GetPrimType());
        break;
      }
      case OP_floor: {
        result = FoldFloor(*cst, node->FromType(), node->GetPrimType());
        break;
      }
      case OP_round: {
        result = FoldRound(*cst, node->FromType(), node->GetPrimType());
        break;
      }
      case OP_trunc: {
        result = FoldTrunk(*cst, node->FromType(), node->GetPrimType());
        break;
      }
      default:
        result = nullptr;
        ASSERT(false, "Unexpected opcode in TypeCvtNodeConstFold");
        break;
    }
  } else if (node->GetOpCode() == OP_cvt && GetPrimTypeSize(node->FromType()) == GetPrimTypeSize(node->GetPrimType())) {
    if ((IsPossible64BitAddress(node->FromType()) && IsPossible64BitAddress(node->GetPrimType())) ||
        (IsPossible32BitAddress(node->FromType()) && IsPossible32BitAddress(node->GetPrimType())) ||
         (IsPrimitivePureScalar(node->FromType()) && IsPrimitivePureScalar(node->GetPrimType()) &&
          GetPrimTypeSize(node->FromType()) == GetPrimTypeSize(node->GetPrimType()))) {
      return p; // the cvt is redundant
    }
  } else if (node->GetOpCode() == OP_cvt && p.second != 0 &&
             IsPrimitiveInteger(node->GetPrimType()) && IsSignedInteger(node->FromType()) &&
             GetPrimTypeSize(node->GetPrimType()) > GetPrimTypeSize(node->FromType())) {
    result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, node->GetPrimType(), node->FromType(), p.first);
    if (IsUnsignedInteger(p.first->GetPrimType())) {
      p.second = static_cast<int32>(p.second);
    }
    return std::make_pair(result, p.second);
  }
  if (result == nullptr) {
    BaseNode *e = PairToExpr(node->Opnd(0)->GetPrimType(), p);
    if (e != node->Opnd(0)) {
      result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(Opcode(node->GetOpCode()),
                                                                 PrimType(node->GetPrimType()),
                                                                 PrimType(node->FromType()), e);
    } else {
      result = node;
    }
  }
  return std::make_pair(result, 0);
}

MIRConst *ConstantFold::FoldSignExtendMIRConst(Opcode opcode, PrimType resultType, uint8 size,
                                               const MIRConst &cst) const {
  const MIRIntConst *constVal = safe_cast<MIRIntConst>(cst);
  ASSERT_NOT_NULL(constVal);
  uint64 result64 = 0;
  if (opcode == OP_sext) {
    result64 = (constVal->GetValue() << (64u - size)) >> (64u - size);
  } else {
    result64 = ((static_cast<uint64>(constVal->GetValue())) << (64u - size)) >> (64u - size);
  }
  MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
  MIRIntConst *constValue =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(result64), type);
  return constValue;
}

ConstvalNode *ConstantFold::FoldSignExtend(Opcode opcode, PrimType resultType, uint8 size,
                                           const ConstvalNode &cst) const {
  ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
  MIRConst *toConst = FoldSignExtendMIRConst(opcode, resultType, size, *cst.GetConstVal());
  resultConst->SetPrimType(toConst->GetType().GetPrimType());
  resultConst->SetConstVal(toConst);
  return resultConst;
}

// sext and zext also handled automatically
std::pair<BaseNode*, int64> ConstantFold::FoldExtractbits(ExtractbitsNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  uint8 offset = node->GetBitsOffset();
  uint8 size = node->GetBitsSize();
  Opcode opcode = node->GetOpCode();
  std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(0));
  ConstvalNode *cst = safe_cast<ConstvalNode>(p.first);
  if (cst != nullptr && (opcode == OP_sext || opcode == OP_zext)) {
    result = FoldSignExtend(opcode, node->GetPrimType(), size, *cst);
  } else {
    BaseNode *e = PairToExpr(node->Opnd(0)->GetPrimType(), p);
    if (e != node->Opnd(0)) {
      result = mirModule->CurFuncCodeMemPool()->New<ExtractbitsNode>(opcode, PrimType(node->GetPrimType()),
                                                                     offset, size, e);
    } else {
      result = node;
    }
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldIread(IreadNode *node) {
  CHECK_NULL_FATAL(node);
  std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(0));
  BaseNode *e = PairToExpr(node->Opnd(0)->GetPrimType(), p);
  node->SetOpnd(e, 0);
  BaseNode *result = node;
  if (e->GetOpCode() != OP_addrof) {
    return std::make_pair(result, 0);
  }

  AddrofNode *addrofNode = static_cast<AddrofNode*>(e);
  MIRSymbol *msy = mirModule->CurFunction()->GetLocalOrGlobalSymbol(addrofNode->GetStIdx());
  TyIdx typeId = msy->GetTyIdx();
  CHECK_FATAL(GlobalTables::GetTypeTable().GetTypeTable().empty() == false, "container check");
  MIRType *msyType = GlobalTables::GetTypeTable().GetTypeTable()[typeId];
  if (addrofNode->GetFieldID() != 0 &&
      (msyType->GetKind() == kTypeStruct || msyType->GetKind() == kTypeClass)) {
    msyType = static_cast<MIRStructType *>(msyType)->GetFieldType(addrofNode->GetFieldID());
  }
  MIRPtrType *ptrType = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(node->GetTyIdx()));
  // If the high level type of iaddrof/iread doesn't match
  // the type of addrof's rhs, this optimization cannot be done.
  if (ptrType->GetPointedType() != msyType) {
    return std::make_pair(result, 0);
  }

  Opcode op = node->GetOpCode();
  FieldID fieldID = node->GetFieldID();
  if (op == OP_iaddrof) {
    AddrofNode *newAddrof = addrofNode->CloneTree(mirModule->GetCurFuncCodeMPAllocator());
    CHECK_NULL_FATAL(newAddrof);
    newAddrof->SetFieldID(newAddrof->GetFieldID() + fieldID);
    result = newAddrof;
  } else if (op == OP_iread) {
    result = mirModule->CurFuncCodeMemPool()->New<AddrofNode>(
          OP_dread, node->GetPrimType(), addrofNode->GetStIdx(), node->GetFieldID() + addrofNode->GetFieldID());
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldBinary(BinaryNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  int64 sum = 0;
  Opcode op = node->GetOpCode();
  PrimType primType = node->GetPrimType();
  PrimType lPrimTypes = node->Opnd(0)->GetPrimType();
  PrimType rPrimTypes = node->Opnd(1)->GetPrimType();
  std::pair<BaseNode*, int64> lp = DispatchFold(node->Opnd(0));
  std::pair<BaseNode*, int64> rp = DispatchFold(node->Opnd(1));
  BaseNode *l = lp.first;
  BaseNode *r = rp.first;
  ConstvalNode *lConst = safe_cast<ConstvalNode>(l);
  ConstvalNode *rConst = safe_cast<ConstvalNode>(r);
  bool isInt = IsPrimitiveInteger(primType);
  if (lConst != nullptr && rConst != nullptr) {
    MIRConst *lConstVal = lConst->GetConstVal();
    MIRConst *rConstVal = rConst->GetConstVal();
    ASSERT_NOT_NULL(lConstVal);
    ASSERT_NOT_NULL(rConstVal);
    // Don't fold div by 0, for floats div by 0 is well defined.
    if ((op == OP_div || op == OP_rem) && isInt &&
        (static_cast<MIRIntConst*>(rConstVal)->GetValue() == 0 ||
         static_cast<MIRIntConst*>(lConstVal)->GetValue() == LONG_MIN ||
         static_cast<MIRIntConst*>(lConstVal)->GetValue() == INT_MIN)) {
      result = NewBinaryNode(node, op, primType, lConst, rConst);
      sum = 0;
    } else {
      // 4 + 2 -> return a pair(result = ConstValNode(6), sum = 0)
      // Create a new ConstvalNode for 6 but keep the sum = 0. This simplify the
      // logic since the alternative is to return pair(result = nullptr, sum = 6).
      // Doing so would introduce many nullptr checks in the code. See previous
      // commits that implemented that logic for a comparison.
      result = FoldConstBinary(op, primType, *lConst, *rConst);
      sum = 0;
    }
  } else if (lConst != nullptr && isInt) {
    MIRIntConst *mcst = safe_cast<MIRIntConst>(lConst->GetConstVal());
    ASSERT_NOT_NULL(mcst);
    PrimType cstTyp = mcst->GetType().GetPrimType();
    int64 cst = mcst->GetValue();
    if (op == OP_add) {
      sum = cst + rp.second;
      result = r;
    } else if (op == OP_sub) {
      sum = cst - rp.second;
      result = NegateTree(r);
    } else if ((op == OP_mul || op == OP_div || op == OP_rem || op == OP_ashr || op == OP_lshr || op == OP_shl ||
                op == OP_band || op == OP_cand || op == OP_land) &&
               cst == 0) {
      // 0 * X -> 0
      // 0 / X -> 0
      // 0 % X -> 0
      // 0 >> X -> 0
      // 0 << X -> 0
      // 0 & X -> 0
      // 0 && X -> 0
      sum = 0;
      result = mirModule->GetMIRBuilder()->CreateIntConst(0, cstTyp);
    } else if (op == OP_mul && cst == 1) {
      // 1 * X --> X
      sum = rp.second;
      result = r;
    } else if (op == OP_bior && cst == -1) {
      // (-1) | X -> -1
      sum = 0;
      result = mirModule->GetMIRBuilder()->CreateIntConst(-1, cstTyp);
    } else if (op == OP_mul && rp.second != 0) {
      // lConst * (X + konst) -> the pair [(lConst*X), (lConst*konst)]
      sum = static_cast<uint64>(cst) * rp.second;
      if (GetPrimTypeSize(primType) > GetPrimTypeSize(rp.first->GetPrimType())) {
        rp.first = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, PTY_i32, rp.first);
      }
      result = NewBinaryNode(node, OP_mul, primType, lConst, rp.first);
    } else if (op == OP_lior || op == OP_cior) {
      sum = 0;
      if (cst != 0) {
        // 5 || X -> 1
        result = mirModule->GetMIRBuilder()->CreateIntConst(1, cstTyp);
      } else {
        // when cst is zero
        // 0 || X -> (X != 0);
        result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(
            OP_ne, primType, r->GetPrimType(), r, mirModule->GetMIRBuilder()->CreateIntConst(0, r->GetPrimType()));
      }
    } else if ((op == OP_cand || op == OP_land) && cst != 0) {
      // 5 && X -> (X != 0)
      sum = 0;
      result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(
          OP_ne, primType, r->GetPrimType(), r, mirModule->GetMIRBuilder()->CreateIntConst(0, r->GetPrimType()));
    } else if ((op == OP_bior || op == OP_bxor) && cst == 0) {
      // 0 | X -> X
      // 0 ^ X -> X
      sum = rp.second;
      result = r;
    } else {
      result = NewBinaryNode(node, op, primType, l, PairToExpr(rPrimTypes, rp));
      sum = 0;
    }
    if (result->GetPrimType() != primType) {
      result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, result->GetPrimType(), result);
    }
  } else if (rConst != nullptr && isInt) {
    MIRIntConst *mcst = safe_cast<MIRIntConst>(rConst->GetConstVal());
    ASSERT_NOT_NULL(mcst);
    PrimType cstTyp = mcst->GetType().GetPrimType();
    int64 cst = mcst->GetValue();
    if (op == OP_add) {
      result = l;
      sum = lp.second + cst;
    } else if (op == OP_sub) {
#if 0  // this is preventing good optimizations
      if (IsSignedInteger(primType) && MinValOfSignedInteger(primType) <= cst) {
        result = node;
        sum = 0;
      } else 
#endif
      {
        result = l;
        sum = lp.second - cst;
      }
    } else if ((op == OP_mul || op == OP_band || op == OP_cand || op == OP_land) && cst == 0) {
      // X * 0 -> 0
      // X & 0 -> 0
      // X && 0 -> 0
      sum = 0;
      result = mirModule->GetMIRBuilder()->CreateIntConst(0, cstTyp);
    } else if ((op == OP_mul || op == OP_div) && cst == 1) {
      // case [X * 1 -> X]
      // case [X / 1 = X]
      sum = lp.second;
      result = l;
    } else if (op == OP_mul && lp.second != 0) {
      // (X + konst) * rConst -> the pair [(X*rConst), (konst*rConst)]
      sum = lp.second * static_cast<uint64>(cst);
      if (GetPrimTypeSize(primType) > GetPrimTypeSize(lp.first->GetPrimType())) {
        lp.first = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, PTY_i32, lp.first);
      }
      result = NewBinaryNode(node, OP_mul, primType, lp.first, rConst);
    } else if (op == OP_band && cst == -1) {
      // X & (-1) -> X
      sum = lp.second;
      result = l;
    } else if (op == OP_bior && cst == -1) {
      // X | (-1) -> -1
      sum = 0;
      result = mirModule->GetMIRBuilder()->CreateIntConst(-1, cstTyp);
    } else if ((op == OP_lior || op == OP_cior)) {
      sum = 0;
      if (cst > 0) {
        // X || 5 -> 1
        result = mirModule->GetMIRBuilder()->CreateIntConst(1, cstTyp);
      } else if (cst == 0) {
        // X || 0 -> X
        sum = lp.second;
        result = l;
      } else {
        result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), r);
      }
    } else if ((op == OP_ashr || op == OP_lshr || op == OP_shl || op == OP_bior || op == OP_bxor) && cst == 0) {
      // X >> 0 -> X
      // X << 0 -> X
      // X | 0 -> X
      // X ^ 0 -> X
      sum = lp.second;
      result = l;
    } else if (op == OP_bxor && cst == 1 && primType != PTY_u1) {
      // bxor i32 (
      //   cvt i32 u1 (regread u1 %13),
      //  constValue i32 1),
      result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), PairToExpr(rPrimTypes, rp));
      sum = 0;
      if (l->GetOpCode() == OP_cvt) {
        TypeCvtNode *cvtNode = static_cast<TypeCvtNode*>(l);
        if (cvtNode->Opnd(0)->GetPrimType() == PTY_u1) {
          BaseNode *base = cvtNode->Opnd(0);
          BaseNode *constValue = mirModule->GetMIRBuilder()->CreateIntConst(1, base->GetPrimType());
          std::pair<BaseNode*, int64> p = DispatchFold(base);
          BinaryNode *temp = NewBinaryNode(node, op, PTY_u1, PairToExpr(base->GetPrimType(), p), constValue);
          result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, PTY_u1, temp);
        }
      }
    } else if (op == OP_rem && cst == 1) {
      // X % 1 -> 0
      sum = 0;
      result = mirModule->GetMIRBuilder()->CreateIntConst(0, cstTyp);
    } else {
      result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), r);
      sum = 0;
    }
    if (result->GetPrimType() != primType) {
      result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, result->GetPrimType(), result);
    }
  } else if (isInt && (op == OP_add || op == OP_sub)) {
    if (op == OP_add) {
      result = NewBinaryNode(node, op, primType, l, r);
      sum = lp.second + rp.second;
    } else {
      result = NewBinaryNode(node, op, primType, l, r);
      sum = lp.second - rp.second;
    }
  } else {
    result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), PairToExpr(rPrimTypes, rp));
    sum = 0;
  }
  return std::make_pair(result, sum);
}

BaseNode *ConstantFold::SimplifyDoubleCompare(CompareNode &compareNode) const {
  // See arm manual B.cond(P2993) and FCMP(P1091)
  CompareNode *node = &compareNode;
  BaseNode *result = node;
  BaseNode *l = node->Opnd(0);
  BaseNode *r = node->Opnd(1);
  if (node->GetOpCode() == OP_ne || node->GetOpCode() == OP_eq) {
    if (l->GetOpCode() == OP_cmp && r->GetOpCode() == OP_constval) {
      ConstvalNode *constNode = static_cast<ConstvalNode*>(r);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
        const CompareNode *compNode = static_cast<CompareNode*>(l);
        result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(Opcode(node->GetOpCode()),
                                                                   PrimType(node->GetPrimType()),
                                                                   compNode->GetOpndType(),
                                                                   compNode->Opnd(0),
                                                                   compNode->Opnd(1));
      }
    } else if (r->GetOpCode() == OP_cmp && l->GetOpCode() == OP_constval) {
      ConstvalNode *constNode = static_cast<ConstvalNode*>(l);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
        const CompareNode *compNode = static_cast<CompareNode*>(r);
        result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(Opcode(node->GetOpCode()),
                                                                   PrimType(node->GetPrimType()),
                                                                   compNode->GetOpndType(),
                                                                   compNode->Opnd(0),
                                                                   compNode->Opnd(1));
      }
    } else if (node->GetOpCode() == OP_ne && r->GetOpCode() == OP_constval) {
      // ne (u1 x, constValue 0)  <==> x
      ConstvalNode *constNode = static_cast<ConstvalNode*>(r);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
        BaseNode *opnd = l;
        do {
          if (opnd->GetPrimType() == PTY_u1 || (l->GetOpCode() == OP_ne || l->GetOpCode() == OP_eq)) {
            result = opnd;
            break;
          } else if (opnd->GetOpCode() == OP_cvt) {
            TypeCvtNode *cvtNode = static_cast<TypeCvtNode*>(opnd);
            opnd = cvtNode->Opnd(0);
          } else {
            opnd = nullptr;
          }
        } while (opnd != nullptr);
      }
    } else if (node->GetOpCode() == OP_eq && r->GetOpCode() == OP_constval) {
      ConstvalNode *constNode = static_cast<ConstvalNode*>(r);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero() &&
          (l->GetOpCode() == OP_ne || l->GetOpCode() == OP_eq)) {
        result = l;
        result->SetOpCode(l->GetOpCode() == OP_ne ? OP_eq : OP_ne);
      }
    }
  } else if (node->GetOpCode() == OP_gt || node->GetOpCode() == OP_lt) {
    if (l->GetOpCode() == OP_cmp && r->GetOpCode() == OP_constval) {
      ConstvalNode *constNode = static_cast<ConstvalNode*>(r);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
        const CompareNode *compNode = static_cast<CompareNode*>(l);
        result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(Opcode(node->GetOpCode()),
                                                                   PrimType(node->GetPrimType()),
                                                                   compNode->GetOpndType(),
                                                                   compNode->Opnd(0),
                                                                   compNode->Opnd(1));
      }
    } else if (r->GetOpCode() == OP_cmp && l->GetOpCode() == OP_constval) {
      ConstvalNode *constNode = static_cast<ConstvalNode*>(l);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
        const CompareNode *compNode = static_cast<CompareNode*>(r);
        result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(Opcode(node->GetOpCode()),
                                                                   PrimType(node->GetPrimType()),
                                                                   compNode->GetOpndType(),
                                                                   compNode->Opnd(1),
                                                                   compNode->Opnd(0));
      }
    }
  }
  return result;
}

std::pair<BaseNode*, int64> ConstantFold::FoldCompare(CompareNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  std::pair<BaseNode*, int64> lp = DispatchFold(node->Opnd(0));
  std::pair<BaseNode*, int64> rp = DispatchFold(node->Opnd(1));
  ConstvalNode *lConst = safe_cast<ConstvalNode>(lp.first);
  ConstvalNode *rConst = safe_cast<ConstvalNode>(rp.first);
  Opcode opcode = node->GetOpCode();
  if (lConst != nullptr && rConst != nullptr && !IsPrimitiveDynType(node->GetOpndType())) {
    result = FoldConstComparison(node->GetOpCode(), node->GetPrimType(), node->GetOpndType(),
                                 *lConst, *rConst);
  } else if (lConst != nullptr && rConst == nullptr && opcode != OP_cmp &&
             lConst->GetConstVal()->GetKind() == kConstInt) {
    BaseNode *l = lp.first;
    BaseNode *r = PairToExpr(node->Opnd(1)->GetPrimType(), rp);
    result = FoldConstComparisonReverse(opcode, node->GetPrimType(), node->GetOpndType(),
                                        *l, *r);
  } else {
    BaseNode *l = PairToExpr(node->Opnd(0)->GetPrimType(), lp);
    BaseNode *r = PairToExpr(node->Opnd(1)->GetPrimType(), rp);
    if (l != node->Opnd(0) || r != node->Opnd(1)) {
      result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(
          Opcode(node->GetOpCode()), PrimType(node->GetPrimType()), PrimType(node->GetOpndType()), l, r);
    } else {
      result = node;
    }
    auto *compareNode = static_cast<CompareNode*>(result);
    CHECK_NULL_FATAL(compareNode);
    result = SimplifyDoubleCompare(*compareNode);
  }
  return std::make_pair(result, 0);
}

BaseNode *ConstantFold::Fold(BaseNode *node) {
  if (node == nullptr || kOpcodeInfo.IsStmt(node->GetOpCode())) {
    return nullptr;
  }
  std::pair<BaseNode*, int64> p = DispatchFold(node);
  BaseNode *result = PairToExpr(node->GetPrimType(), p);
  if (result == node) {
    result = nullptr;
  }
  return result;
}

std::pair<BaseNode*, int64> ConstantFold::FoldDepositbits(DepositbitsNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  uint8 bitsOffset = node->GetBitsOffset();
  uint8 bitsSize = node->GetBitsSize();
  std::pair<BaseNode*, int64> leftPair = DispatchFold(node->Opnd(0));
  std::pair<BaseNode*, int64> rightPair = DispatchFold(node->Opnd(1));
  ConstvalNode *leftConst = safe_cast<ConstvalNode>(leftPair.first);
  ConstvalNode *rightConst = safe_cast<ConstvalNode>(rightPair.first);
  if (leftConst != nullptr && rightConst != nullptr) {
    MIRIntConst *intConst0 = safe_cast<MIRIntConst>(leftConst->GetConstVal());
    MIRIntConst *intConst1 = safe_cast<MIRIntConst>(rightConst->GetConstVal());
    ASSERT_NOT_NULL(intConst0);
    ASSERT_NOT_NULL(intConst1);
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(node->GetPrimType());
    MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(node->GetPrimType());
    MIRIntConst *constValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, type);
    uint64 op0ExtractVal = 0;
    uint64 op1ExtractVal = 0;
    uint64 mask0 = (1LLU << (bitsSize + bitsOffset)) - 1;
    uint64 mask1 = (1LLU << bitsOffset) - 1;
    uint64 op0Mask = ~(mask0 ^ mask1);
    op0ExtractVal = (static_cast<uint64>(intConst0->GetValue()) & op0Mask);
    op1ExtractVal = (static_cast<uint64>(intConst1->GetValue()) << bitsOffset) &
                    ((1ULL << (bitsSize + bitsOffset)) - 1);
    constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        op0ExtractVal | op1ExtractVal, constValue->GetType());
    resultConst->SetConstVal(constValue);
    result = resultConst;
  } else {
    BaseNode *l = PairToExpr(node->Opnd(0)->GetPrimType(), leftPair);
    BaseNode *r = PairToExpr(node->Opnd(1)->GetPrimType(), rightPair);
    if (l != node->Opnd(0) || r != node->Opnd(1)) {
      result = mirModule->CurFuncCodeMemPool()->New<DepositbitsNode>(Opcode(node->GetOpCode()),
                                                                     PrimType(node->GetPrimType()),
                                                                     bitsOffset, bitsSize, l, r);
    } else {
      result = node;
    }
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldArray(ArrayNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = nullptr;
  size_t i = 0;
  bool isFolded = false;
  ArrayNode *arrNode =
      mirModule->CurFuncCodeMemPool()->New<ArrayNode>(*mirModule,
                                                      PrimType(node->GetPrimType()),
                                                      node->GetTyIdx(),
                                                      node->GetBoundsCheck());
  for (i = 0; i < node->GetNopndSize(); i++) {
    std::pair<BaseNode*, int64> p = DispatchFold(node->GetNopndAt(i));
    BaseNode *tmpNode = PairToExpr(node->GetNopndAt(i)->GetPrimType(), p);
    if (tmpNode != node->GetNopndAt(i)) {
      isFolded = true;
    }
    arrNode->GetNopnd().push_back(tmpNode);
    arrNode->SetNumOpnds(arrNode->GetNumOpnds() + 1);
  }
  if (isFolded) {
    result = arrNode;
  } else {
    result = node;
  }
  return std::make_pair(result, 0);
}

std::pair<BaseNode*, int64> ConstantFold::FoldTernary(TernaryNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *result = node;
  std::vector<PrimType> primTypes;
  std::vector<std::pair<BaseNode*, int64>> p;
  for (size_t i = 0; i < node->NumOpnds(); i++) {
    BaseNode *tempNopnd = node->Opnd(i);
    CHECK_NULL_FATAL(tempNopnd);
    primTypes.push_back(tempNopnd->GetPrimType());
    p.push_back(DispatchFold(tempNopnd));
  }
  if (node->GetOpCode() == OP_select) {
    ConstvalNode *const0 = safe_cast<ConstvalNode>(p[0].first);
    if (const0 != nullptr) {
      MIRIntConst *intConst0 = safe_cast<MIRIntConst>(const0->GetConstVal());
      ASSERT_NOT_NULL(intConst0);
      // Selecting the first value if not 0, selecting the second value otherwise.
      if (intConst0->GetValue()) {
        result = PairToExpr(primTypes[1], p[1]);
      } else {
        result = PairToExpr(primTypes[2], p[2]);
      }
    } else if (node->Opnd(0)) {
      ConstvalNode *const1 = safe_cast<ConstvalNode>(p[1].first);
      ConstvalNode *const2 = safe_cast<ConstvalNode>(p[2].first);
      if (const1 != nullptr && const2 != nullptr) {
        MIRIntConst *intConst1 = safe_cast<MIRIntConst>(const1->GetConstVal());
        MIRIntConst *intConst2 = safe_cast<MIRIntConst>(const2->GetConstVal());
        double dconst1 = 0.0;
        double dconst2 = 0.0;
        // for fpconst
        if (intConst1 == nullptr || intConst2 == nullptr) {
          PrimType ptyp = const1->GetPrimType();
          if (ptyp == PTY_f64) {
            MIRDoubleConst *dConst1 = safe_cast<MIRDoubleConst>(const1->GetConstVal());
            dconst1 = dConst1->GetValue();
            MIRDoubleConst *dConst2 = safe_cast<MIRDoubleConst>(const2->GetConstVal());
            dconst2 = dConst2->GetValue();
          } else if (ptyp == PTY_f32) {
            MIRFloatConst *fConst1 = safe_cast<MIRFloatConst>(const1->GetConstVal());
            dconst1 = static_cast<double>(fConst1->GetFloatValue());
            MIRFloatConst *fConst2 = safe_cast<MIRFloatConst>(const2->GetConstVal());
            dconst2 = static_cast<double>(fConst2->GetFloatValue());
          }
        } else {
          dconst1 = static_cast<double>(intConst1->GetValue());
          dconst2 = static_cast<double>(intConst2->GetValue());
        }
        if (dconst1 == 1.0 && dconst2 == 0.0) {
          BaseNode *tmpNode = node->Opnd(0);
          if (node->GetPrimType() != PTY_u1) {
            ConstvalNode *zerokonst = mirModule->GetMIRBuilder()->CreateIntConst(0, node->Opnd(0)->GetPrimType());
            tmpNode = mirModule->CurFuncCodeMemPool()->New<CompareNode>(OP_ne, node->GetPrimType(),
                                                                        node->Opnd(0)->GetPrimType(),
                                                                        node->Opnd(0), zerokonst);
          }
          std::pair<BaseNode*, int64> pairTemp = DispatchFold(tmpNode);
          result = PairToExpr(node->GetPrimType(), pairTemp);
          return std::make_pair(result, 0);
        }
        if (dconst1 == 0.0 && dconst2 == 1.0) {
          BaseNode *lnot = mirModule->CurFuncCodeMemPool()->New<UnaryNode>(OP_lnot, PTY_u1, node->Opnd(0));
          BaseNode *tmpNode = lnot;
          if (node->GetPrimType() != PTY_u1) {
            tmpNode = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, PrimType(node->GetPrimType()),
                                                                        PTY_u1, lnot);
          }
          std::pair<BaseNode*, int64> pairTemp = DispatchFold(tmpNode);
          result = PairToExpr(node->GetPrimType(), pairTemp);
          return std::make_pair(result, 0);
        }
      }
    }
  }
  BaseNode *e0 = PairToExpr(primTypes[0], p[0]);
  BaseNode *e1 = PairToExpr(primTypes[1], p[1]);
  BaseNode *e2 = PairToExpr(primTypes[2], p[2]); // count up to 3 for ternary node
  if (e0 != node->Opnd(0) || e1 != node->Opnd(1) || e2 != node->Opnd(2)) {
    result = mirModule->CurFuncCodeMemPool()->New<TernaryNode>(Opcode(node->GetOpCode()),
                                                                PrimType(node->GetPrimType()),
                                                                e0, e1, e2);
  }
  return std::make_pair(result, 0);
}

StmtNode *ConstantFold::SimplifyDassign(DassignNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  returnValue = Fold(node->GetRHS());
  if (returnValue != nullptr) {
    node->SetRHS(returnValue);
  }
  return node;
}

StmtNode *ConstantFold::SimplifyIassignWithAddrofBaseNode(IassignNode &node, const AddrofNode &base) {
  auto *mirTypeOfIass = GlobalTables::GetTypeTable().GetTypeFromTyIdx(node.GetTyIdx());
  if (!mirTypeOfIass->IsMIRPtrType()) {
    return &node;
  }
  auto *iassPtType = static_cast<MIRPtrType*>(mirTypeOfIass);

  MIRSymbol *lhsSym = mirModule->CurFunction()->GetLocalOrGlobalSymbol(base.GetStIdx());
  TyIdx lhsTyIdx = lhsSym->GetTyIdx();
  if (base.GetFieldID() != 0) {
    auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
    if (!mirType->IsStructType()) {
      return &node;
    }
    lhsTyIdx = static_cast<MIRStructType*>(mirType)->GetFieldType(base.GetFieldID())->GetTypeIndex();
  }
  if (iassPtType->GetPointedTyIdx() == lhsTyIdx) {
    DassignNode *dassignNode = mirModule->CurFuncCodeMemPool()->New<DassignNode>();
    dassignNode->SetStIdx(base.GetStIdx());
    dassignNode->SetRHS(node.GetRHS());
    dassignNode->SetFieldID(base.GetFieldID() + node.GetFieldID());
    return dassignNode;
  }
  return &node;
}

StmtNode *ConstantFold::SimplifyIassignWithIaddrofBaseNode(IassignNode &node, const IaddrofNode &base) {
  auto *mirTypeOfIass = GlobalTables::GetTypeTable().GetTypeFromTyIdx(node.GetTyIdx());
  if (!mirTypeOfIass->IsMIRPtrType()) {
    return &node;
  }
  auto *iassPtType = static_cast<MIRPtrType*>(mirTypeOfIass);

  if (base.GetFieldID() == 0) {
    // this iaddrof is redundant
    node.SetAddrExpr(base.Opnd(0));
    return &node;
  }

  auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(base.GetTyIdx());
  if (!mirType->IsMIRPtrType()) {
    return &node;
  }
  auto *iaddrofPtType = static_cast<MIRPtrType*>(mirType);

  MIRStructType *lhsStructTy =
    static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrofPtType->GetPointedTyIdx()));
  TyIdx lhsTyIdx = lhsStructTy->GetFieldType(base.GetFieldID())->GetTypeIndex();
  if (iassPtType->GetPointedTyIdx() == lhsTyIdx) {
    // eliminate the iaddrof by updating the iassign's fieldID and tyIdx
    node.SetFieldID(node.GetFieldID() + base.GetFieldID());
    node.SetTyIdx(base.GetTyIdx());
    node.SetOpnd(base.Opnd(0), 0);
    // recursive call for the new iassign
    return SimplifyIassign(&node);
  }
  return &node;
}

StmtNode *ConstantFold::SimplifyIassign(IassignNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  returnValue = Fold(node->Opnd(0));
  if (returnValue != nullptr) {
    node->SetOpnd(returnValue, 0);
  }
  returnValue = Fold(node->GetRHS());
  if (returnValue != nullptr) {
    node->SetRHS(returnValue);
  }
  auto *mirTypeOfIass = GlobalTables::GetTypeTable().GetTypeFromTyIdx(node->GetTyIdx());
  if (!mirTypeOfIass->IsMIRPtrType()) {
    return node;
  }

  auto *opnd = node->Opnd(0);
  ASSERT_NOT_NULL(opnd);
  switch (opnd->GetOpCode()) {
    case OP_addrof: {
      return SimplifyIassignWithAddrofBaseNode(*node, static_cast<AddrofNode&>(*opnd));
    }
    case OP_iaddrof: {
      return SimplifyIassignWithIaddrofBaseNode(*node, static_cast<IreadNode&>(*opnd));
    }
    default:
      break;
  }
  return node;
}

StmtNode *ConstantFold::SimplifyCondGoto(CondGotoNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  returnValue = Fold(node->Opnd(0));
  returnValue = (returnValue == nullptr) ? node : returnValue;
  if (node->Opnd(0)->GetOpCode() == OP_select) {
    return SimplifyCondGotoSelect(node);
  } else {
    if (returnValue != node) {
      node->SetOpnd(returnValue, 0);
    }
    ConstvalNode *cst = safe_cast<ConstvalNode>(node->Opnd(0));
    if (cst == nullptr) {
      return node;
    }
    MIRIntConst *intConst = safe_cast<MIRIntConst>(cst->GetConstVal());
    ASSERT_NOT_NULL(intConst);
    if ((node->GetOpCode() == OP_brtrue && intConst->GetValueUnderType() != 0) ||
        (node->GetOpCode() == OP_brfalse && intConst->GetValueUnderType() == 0)) {
      GotoNode *gotoNode = mirModule->CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
      gotoNode->SetOffset(node->GetOffset());
      return gotoNode;
    } else {
      return nullptr;
    }
  }
  return node;
}

StmtNode *ConstantFold::SimplifyCondGotoSelect(CondGotoNode *node) const {
  CHECK_NULL_FATAL(node);
  TernaryNode *sel = static_cast<TernaryNode*>(node->Opnd(0));
  if (sel == nullptr || sel->GetOpCode() != OP_select) {
    return node;
  }
  ConstvalNode *const1 = safe_cast<ConstvalNode>(sel->Opnd(1));
  ConstvalNode *const2 = safe_cast<ConstvalNode>(sel->Opnd(2));
  if (const1 != nullptr && const2 != nullptr) {
    MIRIntConst *intConst1 = safe_cast<MIRIntConst>(const1->GetConstVal());
    MIRIntConst *intConst2 = safe_cast<MIRIntConst>(const2->GetConstVal());
    ASSERT_NOT_NULL(intConst1);
    ASSERT_NOT_NULL(intConst2);
    if (intConst1->GetValue() == 1 && intConst2->GetValue() == 0) {
      node->SetOpnd(sel->Opnd(0), 0);
    } else if (intConst1->GetValue() == 0 && intConst2->GetValue() == 1) {
      node->SetOpCode((node->GetOpCode() == OP_brfalse) ? OP_brtrue : OP_brfalse);
      node->SetOpnd(sel->Opnd(0), 0);
    }
  }
  return node;
}

StmtNode *ConstantFold::SimplifySwitch(SwitchNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  returnValue = Fold(node->GetSwitchOpnd());
  if (returnValue != nullptr) {
    node->SetSwitchOpnd(returnValue);
    ConstvalNode *cst = safe_cast<ConstvalNode>(node->GetSwitchOpnd());
    if (cst == nullptr) {
      return node;
    }
    MIRIntConst *intConst = safe_cast<MIRIntConst>(cst->GetConstVal());
    ASSERT_NOT_NULL(intConst);
    GotoNode *gotoNode = mirModule->CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
    bool isdefault = true;
    for (unsigned i = 0; i < node->GetSwitchTable().size(); i++) {
      if (node->GetCasePair(i).first == intConst->GetValue()) {
        isdefault = false;
        gotoNode->SetOffset((LabelIdx)node->GetCasePair(i).second);
        break;
      }
    }
    if (isdefault) {
      gotoNode->SetOffset(node->GetDefaultLabel());
    }
    return gotoNode;
  }
  return node;
}

StmtNode *ConstantFold::SimplifyUnary(UnaryStmtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  if (node->Opnd(0) == nullptr) {
    return node;
  }
  returnValue = Fold(node->Opnd(0));
  if (returnValue != nullptr) {
    node->SetOpnd(returnValue, 0);
  }
  return node;
}

StmtNode *ConstantFold::SimplifyBinary(BinaryStmtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  returnValue = Fold(node->GetBOpnd(0));
  if (returnValue != nullptr) {
    node->SetBOpnd(returnValue, 0);
  }
  returnValue = Fold(node->GetBOpnd(1));
  if (returnValue != nullptr) {
    node->SetBOpnd(returnValue, 1);
  }
  return node;
}

StmtNode *ConstantFold::SimplifyBlock(BlockNode *node) {
  CHECK_NULL_FATAL(node);
  if (node->GetFirst() == nullptr) {
    return node;
  }
  StmtNode *s = node->GetFirst();
  StmtNode *prevStmt = nullptr;
  do {
    StmtNode *returnValue = Simplify(s);
    if (returnValue != nullptr) {
      if (returnValue->GetOpCode() == OP_block) {
        BlockNode *blk = static_cast<BlockNode*>(returnValue);
        node->ReplaceStmtWithBlock(*s, *blk);
      } else {
        node->ReplaceStmt1WithStmt2(s, returnValue);
      }
      prevStmt = s;
      s = s->GetNext();
    } else {
      // delete s from block
      StmtNode *nextStmt = s->GetNext();
      if (s == node->GetFirst()) {
        node->SetFirst(nextStmt);
        if (nextStmt != nullptr) {
          nextStmt->SetPrev(nullptr);
        }
      } else {
        CHECK_NULL_FATAL(prevStmt);
        prevStmt->SetNext(nextStmt);
        if (nextStmt != nullptr) {
          nextStmt->SetPrev(prevStmt);
        }
      }
      if (s == node->GetLast()) {
        node->SetLast(prevStmt);
      }
      s = nextStmt;
    }
  } while (s != nullptr);
  return node;
}

StmtNode *ConstantFold::SimplifyAsm(AsmNode* node) {
  CHECK_NULL_FATAL(node);
  /* fold constval in input */
  for (size_t i = 0; i < node->NumOpnds(); i++) {
    const std::string &str = GlobalTables::GetUStrTable().GetStringFromStrIdx(node->inputConstraints[i]);
    if (str == "i") {
      std::pair<BaseNode*, int64> p = DispatchFold(node->Opnd(i));
      node->SetOpnd(p.first, i);
      continue;
    }
  }
  return node;
}

StmtNode *ConstantFold::SimplifyIf(IfStmtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  (void)Simplify(node->GetThenPart());
  if (node->GetElsePart()) {
    (void)Simplify(node->GetElsePart());
  }
  returnValue = Fold(node->Opnd());
  if (returnValue != nullptr) {
    node->SetOpnd(returnValue, 0);
    ConstvalNode *cst = safe_cast<ConstvalNode>(node->Opnd());
    // do not delete c/c++ dead if-body here
    if (cst == nullptr || (!mirModule->IsJavaModule())) {
      return node;
    }
    MIRIntConst *intConst = safe_cast<MIRIntConst>(cst->GetConstVal());
    ASSERT_NOT_NULL(intConst);
    if (intConst->GetValue() == 0) {
      return node->GetElsePart();
    } else {
      return node->GetThenPart();
    }
  }
  return node;
}

StmtNode *ConstantFold::SimplifyWhile(WhileStmtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  if (node->Opnd(0) == nullptr) {
    return node;
  }
  if (node->GetBody()) {
    (void)Simplify(node->GetBody());
  }
  returnValue = Fold(node->Opnd(0));
  if (returnValue != nullptr) {
    node->SetOpnd(returnValue, 0);
    ConstvalNode *cst = safe_cast<ConstvalNode>(node->Opnd(0));
    // do not delete c/c++ dead while-body here
    if (cst == nullptr || (!mirModule->IsJavaModule())) {
      return node;
    }
    if (cst->GetConstVal()->IsZero()) {
      if (OP_while == node->GetOpCode()) {
        return nullptr;
      } else {
        return node->GetBody();
      }
    }
  }
  return node;
}

StmtNode *ConstantFold::SimplifyNary(NaryStmtNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  for (size_t i = 0; i < node->NumOpnds(); i++) {
    returnValue = Fold(node->GetNopndAt(i));
    if (returnValue != nullptr) {
      node->SetNOpndAt(i, returnValue);
    }
  }
  return node;
}

StmtNode *ConstantFold::SimplifyIcall(IcallNode *node) {
  CHECK_NULL_FATAL(node);
  BaseNode *returnValue = nullptr;
  for (size_t i = 0; i < node->NumOpnds(); i++) {
    returnValue = Fold(node->GetNopndAt(i));
    if (returnValue != nullptr) {
      node->SetNOpndAt(i, returnValue);
    }
  }
  // icall node transform to call node
  CHECK_FATAL(node->GetNopnd().empty() == false, "container check");
  switch (node->GetNopndAt(0)->GetOpCode()) {
    case OP_addroffunc: {
      AddroffuncNode *addrofNode = static_cast<AddroffuncNode*>(node->GetNopndAt(0));
      CallNode *callNode =
          mirModule->CurFuncCodeMemPool()->New<CallNode>(*mirModule,
                                                         node->GetOpCode() == OP_icall ? OP_call : OP_callassigned);
      if (node->GetOpCode() == OP_icallassigned) {
        callNode->SetReturnVec(node->GetReturnVec());
      }
      callNode->SetPUIdx(addrofNode->GetPUIdx());
      for (size_t i = 1; i < node->GetNopndSize(); i++) {
        callNode->GetNopnd().push_back(node->GetNopndAt(i));
      }
      callNode->SetNumOpnds(callNode->GetNopndSize());
      return callNode;
    }
    default:
      break;
  }
  return node;
}

void ConstantFold::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  mirModule->SetCurFunction(func);
  (void)Simplify(func->GetBody());
}

void M2MConstantFold::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}

bool M2MConstantFold::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(ConstantFold, m);
  if (MeOption::meVerify) {
    VerifyGlobalTypeTable();
  }
  return true;
}
}  // namespace maple
