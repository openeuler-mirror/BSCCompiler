//
// Created by wchenbt on 5/4/2021.
//

#include "san_common.h"

#include "asan_interfaces.h"
#include "me_function.h"
#include "me_ir.h"
#include "mir_builder.h"
#include "string_utils.h"

namespace maple {

void appendToGlobalCtors(const MIRModule &mirModule, const MIRFunction *func) {
  MIRBuilder *mirBuilder = mirModule.GetMIRBuilder();
  MIRFunction *GlobalCtors = mirBuilder->GetOrCreateFunction("__cxx_global_var_init", (TyIdx)PTY_void);
  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  CallNode *callNode = mirBuilder->CreateStmtCall(func->GetPuidx(), args);
  GlobalCtors->GetBody()->AddStatement(callNode);
}

void appendToGlobalDtors(const MIRModule &mirModule, const MIRFunction *func) {
  MIRBuilder *mirBuilder = mirModule.GetMIRBuilder();
  MIRFunction *GlobalCtors = mirBuilder->GetOrCreateFunction("__cxx_global_var_fini", (TyIdx)PTY_void);
  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  CallNode *callNode = mirBuilder->CreateStmtCall(func->GetPuidx(), args);
  GlobalCtors->GetBody()->AddStatement(callNode);
}

MIRFunction *getOrInsertFunction(MIRBuilder *mirBuilder, const char *name, MIRType *retType,
                                 std::vector<MIRType *> argTypes) {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(name);
  MIRFunction *func = mirBuilder->GetOrCreateFunction(name, retType->GetTypeIndex());

  if (strIdx != 0u) {
    return func;
  }

  func->AllocSymTab();

  /* use void* for PTY_dynany */
  if (retType->GetPrimType() == PTY_dynany) {
    retType = GlobalTables::GetTypeTable().GetPtr();
  }

  std::vector<MIRSymbol *> formals;
  for (uint32 j = 0; j < argTypes.size(); ++j) {
    MIRType *argTy = argTypes.at(j);
    /* use void* for PTY_dynany */
    if (argTy->GetPrimType() == PTY_dynany) {
      argTy = GlobalTables::GetTypeTable().GetPtr();
    }
    MIRSymbol *argSt = func->GetSymTab()->CreateSymbol(kScopeLocal);
    const uint32 bufSize = 18;
    char buf[bufSize] = {'\0'};
    int eNum = sprintf_s(buf, bufSize - 1, "p%u", j);
    if (eNum == -1) {
      FATAL(kLncFatal, "sprintf_s failed");
    }
    std::string strBuf(buf);
    argSt->SetNameStrIdx(mirBuilder->GetOrCreateStringIndex(strBuf));
    argSt->SetTyIdx(argTy->GetTypeIndex());
    argSt->SetStorageClass(kScFormal);
    argSt->SetSKind(kStVar);
    func->GetSymTab()->AddToStringSymbolMap(*argSt);
    formals.emplace_back(argSt);
  }
  func->SetAttr(FuncAttrKind::FUNCATTR_public);
  func->SetAttr(FuncAttrKind::FUNCATTR_extern);
  func->UpdateFuncTypeAndFormalsAndReturnType(formals, retType->GetTypeIndex(), false);
  return func;
}

/// Create a global describing a source location.
MIRAddrofConst *createSourceLocConst(MIRModule &mirModule, MIRSymbol *Var, PrimType primType) {
  MIRStrConst *moduleName = createStringConst(mirModule, mirModule.GetFileName(), PTY_a64);
  MIRConst *LocData[] = {
      moduleName,
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(Var->GetSrcPosition().LineNum(),
                                                           *GlobalTables::GetTypeTable().GetInt32()),
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(Var->GetSrcPosition().Column(),
                                                           *GlobalTables::GetTypeTable().GetInt32()),
  };
  // Create struct type
  MIRStructType LocStruct(kTypeStruct);
  GlobalTables::GetTypeTable().AddFieldToStructType(LocStruct, "module_name",
                                                    *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)primType));
  GlobalTables::GetTypeTable().AddFieldToStructType(LocStruct, "line", *GlobalTables::GetTypeTable().GetInt32());
  GlobalTables::GetTypeTable().AddFieldToStructType(LocStruct, "column", *GlobalTables::GetTypeTable().GetInt32());
  // Create initial value
  MIRAggConst *LocStructConst = mirModule.GetMemPool()->New<MIRAggConst>(mirModule, LocStruct);
  // Initialize the field orig
  for (int i = 0; i < 3; i++) {
    LocStructConst->AddItem(LocData[i], i + 1);
  }
  // Create a new symbol, MIRStructType is a subclass of MIRType
  TyIdx LocStructTy = GlobalTables::GetTypeTable().GetOrCreateMIRType(&LocStruct);
  MIRSymbol *LocStructSym = mirModule.GetMIRBuilder()->CreateSymbol(LocStructTy, Var->GetName() + "_Loc", kStConst,
                                                                    kScGlobal, nullptr, kScopeGlobal);
  LocStructSym->SetKonst(LocStructConst);
  return createAddrofConst(mirModule, LocStructSym, primType);
}

MIRAddrofConst *createAddrofConst(const MIRModule &mirModule, const MIRSymbol *mirSymbol, PrimType primType) {
  AddrofNode *addrofNode = mirModule.GetMIRBuilder()->CreateAddrof(*mirSymbol);
  MIRAddrofConst *mirAddrofConst =
      mirModule.GetMemPool()->New<MIRAddrofConst>(addrofNode->GetStIdx(), addrofNode->GetFieldID(),
                                                  *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)primType));
  return mirAddrofConst;
}

// Create a constant for Str so that we can pass it to the run-time lib.
MIRStrConst *createStringConst(const MIRModule &mirModule, std::basic_string<char> Str, PrimType primType) {
  MIRStrConst *strConst =
      mirModule.GetMemPool()->New<MIRStrConst>(GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(Str),
                                               *GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)primType));

  return strConst;
}

bool isTypeSized(MIRType *type) {
  if (type->GetKind() == kTypeScalar || type->GetKind() == kTypePointer || type->GetKind() == kTypeBitField) {
    return true;
  }
  if (type->GetKind() != kTypeStruct && type->GetKind() != kTypeStructIncomplete && type->GetKind() != kTypeArray &&
      type->GetKind() != kTypeFArray && type->GetKind() != kTypeUnion) {
    return false;
  }
  if (type->GetKind() == kTypeArray) {
    MIRArrayType *arrayType = dynamic_cast<MIRArrayType *>(type);
    if (arrayType) {
      return isTypeSized(arrayType->GetElemType());
    }
  }
  if (type->GetKind() == kTypeFArray) {
    MIRFarrayType *farrayType = dynamic_cast<MIRFarrayType *>(type);
    if (farrayType) {
      return isTypeSized(farrayType->GetElemType());
    }
  }
  if (type->IsStructType()) {
    MIRStructType *structType = dynamic_cast<MIRStructType *>(type);
    if (structType) {
      for (size_t i = 1; i < structType->GetFieldsSize(); i++) {
        if (!isTypeSized(structType->GetFieldType(i))) {
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

std::vector<MIRSymbol *> GetGlobalVaribles(const MIRModule &mirModule) {
  std::vector<MIRSymbol *> globalVarVec;
  for (auto sit = mirModule.GetSymbolDefOrder().begin(); sit != mirModule.GetSymbolDefOrder().end(); ++sit) {
    MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx((*sit).Idx());
    CHECK_FATAL(s != nullptr, "nullptr check");
    if (s->IsJavaClassInterface()) {
      continue;
    }
    if (!s->IsDeleted() && !s->GetIsImported() && !s->GetIsImportedDecl()) {
      if (s->GetSKind() == kStVar) {
        globalVarVec.push_back(s);
      }
    }
  }
  return globalVarVec;
}

int computeRedZoneField(MIRType *type) {
  int field = 1;
  if (type->IsStructType()) {
    MIRStructType *structType = dynamic_cast<MIRStructType *>(type);
    for (size_t i = 1; i < structType->GetFieldsSize(); i++) {
      MIRType *subType = structType->GetFieldType(i);
      field += computeRedZoneField(subType);
    }
  }
  return field;
}

size_t TypeSizeToSizeIndex(uint32_t TypeSize) {
  uint32_t Val = TypeSize / 8;
  if (!Val) {
    return std::numeric_limits<uint32_t>::digits;
  }
  if (Val & 0x1) {
    return 0;
  }

  // Bisection method.
  unsigned zeroBits = 0;
  uint32_t shift = std::numeric_limits<uint32_t>::digits >> 1;
  uint32_t mask = std::numeric_limits<uint32_t>::max() >> shift;
  while (shift) {
    if ((Val & mask) == 0) {
      Val >>= shift;
      zeroBits |= shift;
    }
    shift >>= 1;
    mask >>= shift;
  }
  assert(zeroBits < kNumberOfAccessSizes);
  return zeroBits;
}

MIRSymbol *getOrCreateSymbol(MIRBuilder *mirBuilder, TyIdx tyIdx, const std::string &name, MIRSymKind mClass,
                             MIRStorageClass sClass, MIRFunction *func, uint8 scpID) {
  MIRSymbol *st = nullptr;
  if (func) {
    st = func->GetSymTab()->GetSymbolFromStrIdx(mirBuilder->GetOrCreateStringIndex(name));
  } else {
    st = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(mirBuilder->GetOrCreateStringIndex(name));
  }
  if (st == nullptr || st->GetTyIdx() != tyIdx) {
    return mirBuilder->CreateSymbol(tyIdx, name, mClass, sClass, func, scpID);
  }
  ASSERT(mClass == st->GetSKind(),
         "trying to create a new symbol that has the same name and GtyIdx. might cause problem");
  ASSERT(sClass == st->GetStorageClass(),
         "trying to create a new symbol that has the same name and tyIdx. might cause problem");
  return st;
}

// Code for Sanrazor
int SANRAZOR_MODE() {
  /*
  Sanrazor has several mode
  0. Didn't turn on / default
  1. intrument for coverage
  2. collected coverage, analysis and remove sanitzer check 
  */
  char *env = getenv("SANRAZOR_MODE");
  int SanrazorMode = 0;
  if (env) {
    SanrazorMode = atoi(env);
    if (SanrazorMode >= 3) {
      return 0;
    }
    return SanrazorMode;
  } else {
    return 0;
  }
}

CallNode *retCallCOV(const MeFunction &func, int bb_id, int stmt_id, int br_true, int type_of_check) {
  MIRBuilder *builder = func.GetMIRModule().GetMIRBuilder();
  MIRType *voidType = GlobalTables::GetTypeTable().GetVoid();
  // void __san_cov_trace_pc(char *file_name, int bb_id, int stmt_id,int brtrue,int typecheck)
  MIRFunction *__san_cov_trace_pc = getOrInsertFunction(builder, "__san_cov_trace_pc", voidType, {});
  MapleVector<BaseNode *> argcov(func.GetMIRModule().GetMPAllocator().Adapter());
  UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(func.GetMIRModule().GetFileName());
  ConststrNode *conststr = func.GetMIRModule().GetMemPool()->New<ConststrNode>(strIdx);
  conststr->SetPrimType(PTY_a64);
  argcov.emplace_back(conststr);
  argcov.emplace_back(builder->GetConstInt(bb_id));
  argcov.emplace_back(builder->GetConstInt(stmt_id));
  argcov.emplace_back(builder->GetConstInt(br_true));
  argcov.emplace_back(builder->GetConstInt(type_of_check));
  CallNode *callcov = builder->CreateStmtCall(__san_cov_trace_pc->GetPuidx(), argcov);
  return callcov;
}

bool isReg_redefined(BaseNode *stmt, std::vector<PregIdx> &stmt_reg) {
  switch (stmt->GetOpCode()) {
    case OP_regread: {
      RegreadNode *regread = static_cast<RegreadNode *>(stmt);
      stmt_reg.push_back(regread->GetRegIdx());
      break;
    }
    default: {
      for (size_t i = 0; i < stmt->NumOpnds(); i++) {
        isReg_redefined(stmt->Opnd(i), stmt_reg);
      }
    }
  }
  if (stmt->GetOpCode() == OP_regassign) {
    RegassignNode *regAssign = static_cast<RegassignNode *>(stmt);
    if (std::count(stmt_reg.begin(), stmt_reg.end(), regAssign->GetRegIdx())) {
      // value update
      return false;
    } else {
      return true;
    }
  }
  return false;
}

template <typename T>
void print_stack(std::stack<T> &st) {
  if (st.empty()) return;
  T x = st.top();
  LogInfo::MapleLogger() << x << ",";
  st.pop();
  print_stack(st);
  st.push(x);
}

template <typename T>
bool compareVectors(std::vector<T> a, std::vector<T> b) {
  // I am not sure why the original implementation use
  // sets to compare the equivalence of two vectors (peformance?)
  // Anyway, I think we may not delete the following code now
  // if (a.size() != b.size())
  // {
  //    return false;
  // }
  // std::sort(a.begin(), a.end());
  // std::sort(b.begin(), b.end());
  // return (a == b);
  std::set<T> set_a(a.begin(), a.end());
  std::set<T> set_b(b.begin(), b.end());
  if ((set_a.size() > 0) && (set_b.size() > 0)) {
    return (set_a == set_b);
  }
  return false;
}

int getIndex(std::vector<StmtNode *> v, StmtNode *K) {
  auto it = find(v.begin(), v.end(), K);
  // If element was found
  if (it != v.end()) {
    int index = it - v.begin();
    return index;
  } else {
    return -1;
  }
}

StmtNode *retLatest_Regassignment(StmtNode *stmt, int32 register_number) {
  StmtNode *ret_stmt = nullptr;
  StmtNode *prevStmt = stmt->GetPrev();
  if (prevStmt != nullptr) {
    if (prevStmt->GetOpCode() == OP_regassign) {
      RegassignNode *regAssign = static_cast<RegassignNode *>(prevStmt);
      if (register_number == regAssign->GetRegIdx()) {
        return prevStmt;
      } else {
        ret_stmt = retLatest_Regassignment(prevStmt, register_number);
      }
    } else if (prevStmt->GetOpCode() == OP_iassign) {
      IassignNode *iassign = static_cast<IassignNode *>(prevStmt);
      BaseNode *addr_expr = iassign->Opnd(0);
      if (addr_expr->GetOpCode() == OP_iread) {
        std::vector<int32> dump_reg;
        recursion(addr_expr, dump_reg);
        for (int32 reg_tmp : dump_reg) {
          if (reg_tmp == register_number) {
            return prevStmt;
          }
        }
        ret_stmt = retLatest_Regassignment(prevStmt, register_number);
      } else if (addr_expr->GetOpCode() == OP_regread) {
        RegreadNode *regread = static_cast<RegreadNode *>(addr_expr);
        if (register_number == regread->GetRegIdx()) {
          return prevStmt;
        } else {
          ret_stmt = retLatest_Regassignment(prevStmt, register_number);
        }
      } else if (IsCommutative(addr_expr->GetOpCode())) {
        /*
          0th stmt: add u64 (
            iread u64 <* <$_TY_IDX111>> 22 (regread ptr %177),
            cvt u64 i32 (mul i32 (regread i32 %190, constval i32 2)))
          */
        // We just assume its sth like register +/- sth patterns
        std::vector<int32> dump_reg;
        recursion(addr_expr->Opnd(0), dump_reg);
        for (int32 reg_tmp : dump_reg) {
          if (reg_tmp == register_number) {
            return prevStmt;
          }
        }
        ret_stmt = retLatest_Regassignment(prevStmt, register_number);
      } else {
        ret_stmt = retLatest_Regassignment(prevStmt, register_number);
      }
    } else {
      ret_stmt = retLatest_Regassignment(prevStmt, register_number);
    }
  }
  return ret_stmt;
}
StmtNode *retLatest_Varassignment(StmtNode *stmt, uint32 var_number) {
  StmtNode *ret_stmt = nullptr;
  StmtNode *prevStmt = stmt->GetPrev();
  if (prevStmt != nullptr) {
    if (prevStmt->GetOpCode() == OP_dassign || prevStmt->GetOpCode() == OP_maydassign) {
      DassignNode *dassign = static_cast<DassignNode *>(prevStmt);
      if (var_number == dassign->GetStIdx().Idx()) {
        return prevStmt;
      } else {
        ret_stmt = retLatest_Varassignment(prevStmt, var_number);
      }
    } else if (prevStmt->GetOpCode() == OP_iassign) {
      IassignNode *iassign = static_cast<IassignNode *>(prevStmt);
      BaseNode *addr_expr = iassign->Opnd(0);
      if (addr_expr->GetOpCode() == OP_dread) {
        // dread i64 %asan_shadowBase
        DreadNode *dread = static_cast<DreadNode *>(addr_expr);
        if (var_number == dread->GetStIdx().Idx()) {
          return prevStmt;
        } else {
          ret_stmt = retLatest_Varassignment(prevStmt, var_number);
        }
      } else {
        ret_stmt = retLatest_Varassignment(prevStmt, var_number);
      }
    } else {
      ret_stmt = retLatest_Varassignment(prevStmt, var_number);
    }
  }
  return ret_stmt;
}

void print_dep(set_check dep) {
  LogInfo::MapleLogger() << "\nOpcode: ";
  for (auto opcode_tmp : dep.opcode) {
    LogInfo::MapleLogger() << int(opcode_tmp) << ",";
  }
  LogInfo::MapleLogger() << "\n";
}

std::set<Opcode> OP_code_blacklist{
    OP_addroffunc, OP_iaddrof, OP_addrof, OP_iread, OP_ireadoff, OP_iassign, OP_dread, OP_regread, OP_regassign,
    OP_dassign, OP_maydassign, OP_iassignoff, OP_iassignfpoff,
    //  We only handle the SAN-SAN case
    //  // check with edit distance ==1
    OP_cvt,
    // candidnate:
    //  OP_band,
    //  OP_zext,
    //  OP_ashr,
    //  // check with edit distance ==2
    //  OP_add,
    //  OP_sub,
    OP_constval,
    // candidnate:
    //  OP_add,
    //  OP_ashr
};

std::set<Opcode> OP_code_re_map{OP_eq, OP_ge, OP_gt, OP_le, OP_lt, OP_ne, OP_cmp, OP_cmpl, OP_cmpg};

void dep_expansion(BaseNode *stmt, set_check &dep, std::map<int32, std::vector<StmtNode *>> reg_to_stmt,
                   std::map<uint32, std::vector<StmtNode *>> var_to_stmt, MeFunction func) {
  if ((!OP_code_blacklist.count(stmt->GetOpCode())) && (!OP_code_re_map.count(stmt->GetOpCode()))) {
    dep.opcode.push_back(stmt->GetOpCode());
  } else if (OP_code_re_map.count(stmt->GetOpCode())) {
    dep.opcode.push_back(uint8(253));
  }
  switch (stmt->GetOpCode()) {
    case OP_iassign: {
      IassignNode *iassign = static_cast<IassignNode *>(stmt);
      BaseNode *rhs_expr = iassign->Opnd(1);
      if (rhs_expr->GetOpCode() == OP_regread) {
        // Case 1. regread u32 %13
        RegreadNode *regread = static_cast<RegreadNode *>(rhs_expr);
        dep.register_live.push(regread->GetRegIdx());
      } else if (rhs_expr->GetOpCode() == OP_constval) {
        // Case 2. constval i32 0 -> terminal
        ConstvalNode *constValNode = static_cast<ConstvalNode *>(rhs_expr);
        MIRConst *mirConst = constValNode->GetConstVal();
        if (mirConst != nullptr) {
          if (mirConst->GetKind() == kConstInt) {
            auto *const_to_get_value = safe_cast<MIRIntConst>(mirConst);
            dep.const_int64.push_back(const_to_get_value->GetValue());
          }
        }
      } else if (rhs_expr->GetOpCode() == OP_iread) {
        // Case 3. iread agg <* <$_TY_IDX334>> 0 (regread ptr %4) -> Only hold the ptr for deref
        std::vector<int32> dump_reg;
        recursion(rhs_expr, dump_reg);
        for (int32 reg_temp : dump_reg) {
          dep.register_live.push(reg_temp);
        }
      } else {
        // Case 4. zext u32 8 (lshr u32 (regread u32 %4, constval i32 24))
        // Just assume it can be further expand and treat as a terminal...
        // Some of this of compound stmt are register
        // assigned by callassigned or function input register
        // Although there are some case didn't like this
        // We can set it as terminal register to prevent recursively deref
        // since it may crash
        // A proper SSA likely fix this issue
        std::vector<int32> dump_reg;
        recursion(rhs_expr, dump_reg);
        for (int32 reg_temp : dump_reg) {
          dep.register_terminal.push_back(reg_temp);
        }
      }
      break;
    }
    case OP_regread: {
      RegreadNode *regread = static_cast<RegreadNode *>(stmt);
      dep.register_live.push(regread->GetRegIdx());
      break;
    }
    case OP_constval: {
      ConstvalNode *constValNode = static_cast<ConstvalNode *>(stmt);
      MIRConst *mirConst = constValNode->GetConstVal();
      // we only trace int64
      // We didn't handle following cases
      // kConstFloatConst, MIRFloatConst
      // kConstDoubleConst, MIRDoubleConst
      if (mirConst != nullptr) {
        if (mirConst->GetKind() == kConstInt) {
          auto *const_to_get_value = safe_cast<MIRIntConst>(mirConst);
          dep.const_int64.push_back(const_to_get_value->GetValue());
        }
      }
      break;
    }
    case OP_conststr: {
      ConststrNode *conststr = static_cast<ConststrNode *>(stmt);
      dep.const_str.push_back(conststr->GetStrIdx());
      break;
    }
    case OP_conststr16: {
      Conststr16Node *conststr16 = static_cast<Conststr16Node *>(stmt);
      dep.const_str.push_back(conststr16->GetStrIdx());
      break;
    }
    case OP_dread: {
      DreadNode *dread = static_cast<DreadNode *>(stmt);
      dep.var_live.push(dread->GetStIdx().Idx());
      break;
    }
    case OP_addrof: {
      AddrofNode *addrof = static_cast<AddrofNode *>(stmt);
      dep.var_live.push(addrof->GetStIdx().Idx());
      break;
    }
    case OP_addroffunc: {
      // We don't handle function pointer
      break;
    }
    case OP_dassign: {
      DassignNode *dassign = static_cast<DassignNode *>(stmt);
      std::stack<uint8> san_blacklist_stack;
      bool required_to_clean_san = false;
      if (func.GetMIRModule().CurFunction()->GetSymbolTabSize() >= int(dassign->GetStIdx().Idx())) {
        MIRSymbol *var = func.GetMIRModule().CurFunction()->GetSymbolTabItem(dassign->GetStIdx().Idx());
        if (var->GetName().find("asan_length") == 0) {
          // dassign %asan_length 0 (band i64 (dread i64 %asan_addr, constval i64 7))
          san_blacklist_stack.push(OP_band);
          san_blacklist_stack.push(OP_add);
          required_to_clean_san = true;
        } else if (var->GetName().find("asan_shadowValue") == 0) {
          san_blacklist_stack.push(OP_ashr);
          san_blacklist_stack.push(OP_add);
          required_to_clean_san = true;
        }
      }
      if (required_to_clean_san) {
        std::vector<std::vector<uint8>::iterator> it_vector;
        if (san_blacklist_stack.size() >= dep.opcode.size()) {
          for (int opcode_vect_i = 0; opcode_vect_i < dep.opcode.size(); ++opcode_vect_i) {
            dep.opcode.pop_back();
          }
        } else {
          while (!san_blacklist_stack.empty()) {
            bool done = false;
            uint8 remove_item = san_blacklist_stack.top();
            san_blacklist_stack.pop();
            LogInfo::MapleLogger() << remove_item;
            for (std::vector<uint8>::iterator it = dep.opcode.begin(); it != dep.opcode.end(); ++it) {
              if (*it == remove_item && !done) {
                dep.opcode.erase(it);
                done = true;
              }
            }
          }
        }
      }
      for (size_t i = 0; i < stmt->NumOpnds(); i++) {
        dep_expansion(stmt->Opnd(i), dep, reg_to_stmt, var_to_stmt, func);
      }
      break;
    }
    case OP_dassignoff: {
      // TODO:
      // It is not documented in MAPLE IR.
      break;
    }
    default: {
      for (size_t i = 0; i < stmt->NumOpnds(); i++) {
        dep_expansion(stmt->Opnd(i), dep, reg_to_stmt, var_to_stmt, func);
      }
      break;
    }
  }
}

set_check commit(set_check old, set_check latest) {
  old.opcode.insert(old.opcode.end(), latest.opcode.begin(), latest.opcode.end());
  old.register_terminal.insert(old.register_terminal.end(), latest.register_terminal.begin(),
                               latest.register_terminal.end());
  old.var_terminal.insert(old.var_terminal.end(), latest.var_terminal.begin(), latest.var_terminal.end());
  old.const_int64.insert(old.const_int64.end(), latest.const_int64.begin(), latest.const_int64.end());
  old.const_str.insert(old.const_str.end(), latest.const_str.begin(), latest.const_str.end());
  old.type_num.insert(old.type_num.end(), latest.type_num.begin(), latest.type_num.end());
  return old;
}

bool sat_check(set_check a, set_check b) {
  if (compareVectors(a.opcode, b.opcode)
      /*
      A strict check should also check
        compareVectors(a.register_terminal,b.register_terminal)
        compareVectors(a.var_terminal,b.var_terminal)
        compareVectors(a.const_int64,b.const_int64)
      */
  ) {
    return true;
  }
  return false;
}

void gen_register_dep(StmtNode *stmt, set_check &br_tmp, std::map<int32, std::vector<StmtNode *>> reg_to_stmt,
                      std::map<uint32, std::vector<StmtNode *>> var_to_stmt, MeFunction func) {
  while (!br_tmp.register_live.empty()) {
    int32 register_to_check = br_tmp.register_live.top();
    auto iter = reg_to_stmt.find(register_to_check);
    br_tmp.register_live.pop();
    if (iter != reg_to_stmt.end()) {
      std::vector<StmtNode *> tmp = reg_to_stmt[register_to_check];
      StmtNode *latest_stmt_tmp = retLatest_Regassignment(stmt, register_to_check);
      if (latest_stmt_tmp != nullptr) {
        set_check br_tmp_go;
        dep_expansion(latest_stmt_tmp, br_tmp_go, reg_to_stmt, var_to_stmt, func);
        gen_register_dep(latest_stmt_tmp, br_tmp_go, reg_to_stmt, var_to_stmt, func);
        br_tmp = commit(br_tmp, br_tmp_go);
      }
    } else {
      br_tmp.register_terminal.push_back(register_to_check);
    }
  }
  while (!br_tmp.var_live.empty()) {
    uint32 var_to_check = br_tmp.var_live.top();
    auto iter = var_to_stmt.find(var_to_check);
    br_tmp.var_live.pop();
    if (iter != var_to_stmt.end()) {
      StmtNode *latest_stmt_tmp = retLatest_Varassignment(stmt, var_to_check);
      if (latest_stmt_tmp != nullptr) {
        set_check br_tmp_go_var;
        dep_expansion(latest_stmt_tmp, br_tmp_go_var, reg_to_stmt, var_to_stmt, func);
        gen_register_dep(latest_stmt_tmp, br_tmp_go_var, reg_to_stmt, var_to_stmt, func);
        br_tmp = commit(br_tmp, br_tmp_go_var);
      }
    } else {
      br_tmp.var_terminal.push_back(var_to_check);
    }
  }
}

bool isVar_redefined(BaseNode *stmt, std::vector<uint32> &stmt_reg) {
  switch (stmt->GetOpCode()) {
    case OP_dread: {
      DreadNode *dread = static_cast<DreadNode *>(stmt);
      stmt_reg.push_back(dread->GetStIdx().Idx());
      break;
    }
    default: {
      for (size_t i = 0; i < stmt->NumOpnds(); i++) {
        isVar_redefined(stmt->Opnd(i), stmt_reg);
      }
    }
  }
  if (stmt->GetOpCode() == OP_dassign || stmt->GetOpCode() == OP_maydassign) {
    DassignNode *dassign = static_cast<DassignNode *>(stmt);
    if (std::count(stmt_reg.begin(), stmt_reg.end(), dassign->GetStIdx().Idx())) {
      // value update
      return false;
    } else {
      return true;
    }
  }
  return false;
}

void recursion(BaseNode *stmt, std::vector<int32> &stmt_reg) {
  switch (stmt->GetOpCode()) {
    case OP_regread: {
      RegreadNode *regread = static_cast<RegreadNode *>(stmt);
      stmt_reg.push_back(regread->GetRegIdx());
      break;
    }
    default: {
      for (size_t i = 0; i < stmt->NumOpnds(); i++) {
        recursion(stmt->Opnd(i), stmt_reg);
      }
    }
  }
}

// stmtID to reduciable stmt ID
std::map<int, san_struct> gen_dynmatch(std::string file_name) {
  // read log files and parse the stmtID with br information
  FILE *fp;
  auto log_name = file_name + ".log";
  // to hold the temp data
  std::map<int, san_struct> ret_log_update;

  fp = fopen(("./" + log_name).c_str(), "r");
  if (fp == nullptr) {
    abort();
  }
  // 1. Parse SAN-SAN
  while (true) {
    int cur_id;
    int rc = fscanf_s(fp, "%d", &cur_id, sizeof(cur_id));
    if (rc != 1) {
      break;
    }
    int stmt_ID_cur = cur_id >> 1;
    int br_true_tmp = (stmt_ID_cur << 1) ^ cur_id;
    if (ret_log_update.count(stmt_ID_cur)) {
      // L:0, R:1
      if (br_true_tmp == 1) {
        ret_log_update[stmt_ID_cur].r_ctr += 1;
      } else {
        ret_log_update[stmt_ID_cur].l_ctr += 1;
      }
      ret_log_update[stmt_ID_cur].tot_ctr += 1;
    } else {
      san_struct tmp_san_struct;
      tmp_san_struct.stmtID = stmt_ID_cur;
      tmp_san_struct.tot_ctr = 1;
      tmp_san_struct.l_ctr = 0;
      tmp_san_struct.r_ctr = 0;
      if (br_true_tmp == 1) {
        tmp_san_struct.r_ctr += 1;
      } else {
        tmp_san_struct.l_ctr += 1;
      }
      ret_log_update[stmt_ID_cur] = tmp_san_struct;
    }
  }
  fclose(fp);
  return ret_log_update;
}

bool dynamic_sat(san_struct a, san_struct b, bool SCSC) {
  // For SC-UC case, SC must be var a
  if (a.tot_ctr == b.tot_ctr) {
    if ((a.l_ctr == b.l_ctr) || (a.l_ctr == b.r_ctr)) {
      return true;
    } else {
      return false;
    }
  } else if (!SCSC) {
    // true is 0
    if (a.tot_ctr == b.l_ctr) {
      if ((a.tot_ctr == a.l_ctr) || (a.tot_ctr == a.r_ctr)) {
        return true;
      } else {
        return false;
      }
    } else if (a.tot_ctr == b.r_ctr) {
      if ((a.tot_ctr == a.l_ctr) || (a.tot_ctr == a.r_ctr)) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
    return false;
  }
}

}  // namespace maple