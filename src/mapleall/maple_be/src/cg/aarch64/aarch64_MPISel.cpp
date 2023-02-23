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

#include "aarch64_memlayout.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"
#include "isel.h"
#include "aarch64_MPISel.h"

namespace maplebe {
/* local Handle functions in isel, do not delete or move */
void HandleGoto(StmtNode &stmt, MPISel &iSel);
void HandleLabel(StmtNode &stmt, const MPISel &iSel);

void AArch64MPIsel::HandleFuncExit() const {
  BlockNode *block = cgFunc->GetFunction().GetBody();
  ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
  cgFunc->GetCurBB()->SetLastStmt(*block->GetLast());
  /* Set lastbb's frequency */
  cgFunc->SetLastBB(*cgFunc->GetCurBB());
}

/* Field-ID 0 is assigned to the top level structure. (Field-ID also defaults to 0 if it is not a structure.) */
MemOperand &AArch64MPIsel::GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId,
    RegOperand *baseReg) {
  PrimType symType;
  uint64 fieldOffset = 0;
  bool isCopy = IsParamStructCopy(symbol);
  if (fieldId == 0) {
    symType = symbol.GetType()->GetPrimType();
  } else {
    MIRType *mirType = symbol.GetType();
    ASSERT((mirType->IsMIRStructType() || mirType->IsMIRUnionType()), "non-structure");
    MIRStructType *structType = static_cast<MIRStructType*>(mirType);
    symType = structType->GetFieldType(fieldId)->GetPrimType();
    if (baseReg || !isCopy) {
      fieldOffset = static_cast<uint64>(cgFunc->GetBecommon().GetFieldOffset(*structType, fieldId).first);
    }
  }
  uint32 opndSz = (symType == PTY_agg) ? k64BitSize : GetPrimTypeBitSize(symType);
  if (isCopy) {
    opndSz = (baseReg) ? opndSz : k64BitSize;
  }
  if (baseReg) {
    AArch64CGFunc *a64func = static_cast<AArch64CGFunc*>(cgFunc);
    OfstOperand *ofstOpnd = &a64func->GetOrCreateOfstOpnd(fieldOffset, k32BitSize);
    return *a64func->CreateMemOperand(opndSz, *baseReg, *ofstOpnd);
  } else {
    return GetOrCreateMemOpndFromSymbol(symbol, opndSz, fieldOffset);
  }
}
MemOperand &AArch64MPIsel::GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) const {
  return static_cast<AArch64CGFunc*>(cgFunc)->GetOrCreateMemOpnd(symbol, offset, opndSize);
}

Operand *AArch64MPIsel::SelectFloatingConst(MIRConst &mirConst, PrimType primType, const BaseNode &parent) const {
  CHECK_FATAL(primType == PTY_f64 || primType == PTY_f32, "wrong const");
  AArch64CGFunc *a64Func = static_cast<AArch64CGFunc*>(cgFunc);
  if (primType == PTY_f64) {
    auto *dblConst = safe_cast<MIRDoubleConst>(mirConst);
    return a64Func->HandleFmovImm(primType, dblConst->GetIntValue(), *dblConst, parent);
  } else {
    auto *floatConst = safe_cast<MIRFloatConst>(mirConst);
    return a64Func->HandleFmovImm(primType, floatConst->GetIntValue(), *floatConst, parent);
  }
}

void AArch64MPIsel::SelectReturn(NaryStmtNode &retNode) {
  ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
  Operand *opnd = nullptr;
  if (retNode.NumOpnds() != 0) {
    if (!cgFunc->GetFunction().StructReturnedInRegs()) {
      opnd = cgFunc->HandleExpr(retNode, *retNode.Opnd(0));
    } else {
      cgFunc->SelectReturnSendOfStructInRegs(retNode.Opnd(0));
    }
  }
  cgFunc->SelectReturn(opnd);
}

void AArch64MPIsel::SelectReturn(bool noOpnd) {
  /* if return operand exist, cgFunc->SelectReturn will generate it */
  if (noOpnd) {
    MOperator mOp = MOP_xuncond;
    LabelOperand &targetOpnd = cgFunc->GetOrCreateLabelOperand(cgFunc->GetReturnLabel()->GetLabelIdx());
    Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, AArch64CG::kMd[mOp]);
    jmpInsn.AddOpndChain(targetOpnd);
    cgFunc->GetCurBB()->AppendInsn(jmpInsn);
  }
}

void AArch64MPIsel::CreateCallStructParamPassByStack(const MemOperand &memOpnd, uint32 symSize, int32 baseOffset) {
  uint32 copyTime = RoundUp(symSize, GetPointerSize()) / GetPointerSize();
  for (uint32 i = 0; i < copyTime; ++i) {
    MemOperand &addrMemOpnd = cgFunc->GetOpndBuilder()->CreateMem(k64BitSize);
    addrMemOpnd.SetBaseRegister(*memOpnd.GetBaseRegister());
    ImmOperand &newImmOpnd = static_cast<ImmOperand&>(*memOpnd.GetOffsetOperand()->Clone(*cgFunc->GetMemoryPool()));
    newImmOpnd.SetValue(newImmOpnd.GetValue() + i * GetPointerSize());
    addrMemOpnd.SetOffsetOperand(newImmOpnd);
    RegOperand &spOpnd = cgFunc->GetOpndBuilder()->CreatePReg(RSP, k64BitSize, kRegTyInt);
    Operand &stMemOpnd = cgFunc->GetOpndBuilder()->CreateMem(spOpnd,
        (baseOffset + i * GetPointerSize()), k64BitSize);
    SelectCopy(stMemOpnd, addrMemOpnd, PTY_u64);
  }
}

void AArch64MPIsel::CreateCallStructParamPassByReg(const MemOperand &memOpnd, regno_t regNo, uint32 parmNum) {
  RegOperand &parmOpnd = cgFunc->GetOpndBuilder()->CreatePReg(regNo, k64BitSize, kRegTyInt);
  MemOperand &addrMemOpnd = cgFunc->GetOpndBuilder()->CreateMem(k64BitSize);
  addrMemOpnd.SetBaseRegister(*memOpnd.GetBaseRegister());
  ImmOperand &newImmOpnd = static_cast<ImmOperand&>(*memOpnd.GetOffsetOperand()->Clone(*cgFunc->GetMemoryPool()));
  newImmOpnd.SetValue(newImmOpnd.GetValue() + parmNum * GetPointerSize());
  addrMemOpnd.SetOffsetOperand(newImmOpnd);
  paramPassByReg.push_back({&parmOpnd, &addrMemOpnd, PTY_a64});
}

std::tuple<Operand*, size_t, MIRType*> AArch64MPIsel::GetMemOpndInfoFromAggregateNode(BaseNode &argExpr) {
  /* get mirType info */
  auto [fieldId, mirType] = GetFieldIdAndMirTypeFromMirNode(argExpr);
  MirTypeInfo symInfo = GetMirTypeInfoFormFieldIdAndMirType(fieldId, mirType);
  /* get symbol memOpnd info */
  MemOperand *symMemOpnd = nullptr;
  if (argExpr.GetOpCode() == OP_dread) {
    AddrofNode &dread = static_cast<AddrofNode&>(argExpr);
    MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(dread.GetStIdx());
    symMemOpnd = &GetOrCreateMemOpndFromSymbol(*symbol, dread.GetFieldID());
  } else if (argExpr.GetOpCode() == OP_iread) {
    IreadNode &iread = static_cast<IreadNode&>(argExpr);
    symMemOpnd = GetOrCreateMemOpndFromIreadNode(iread, symInfo.primType, symInfo.offset);
  } else {
    CHECK_FATAL_FALSE("unsupported opcode");
  }
  return {symMemOpnd, symInfo.size, mirType};
}

void AArch64MPIsel::SelectParmListForAggregate(BaseNode &argExpr, AArch64CallConvImpl &parmLocator, bool isArgUnused) {
  auto [argOpnd, argSize, mirType] = GetMemOpndInfoFromAggregateNode(argExpr);
  ASSERT(argOpnd->IsMemoryAccessOperand(), "wrong opnd");
  MemOperand &memOpnd = static_cast<MemOperand&>(*argOpnd);

  CCLocInfo ploc;
  parmLocator.LocateNextParm(*mirType, ploc);
  if (isArgUnused) {
    return;
  }

  /* create call struct param pass */
  if (argSize > k16ByteSize || ploc.reg0 == kRinvalid) {
    CreateCallStructParamPassByStack(memOpnd, argSize, ploc.memOffset);
  } else {
    CHECK_FATAL(ploc.fpSize == 0, "Unknown call parameter state");
    CreateCallStructParamPassByReg(memOpnd, ploc.reg0, 0);
    if (ploc.reg1 != kRinvalid) {
      CreateCallStructParamPassByReg(memOpnd, ploc.reg1, kSecondReg);
    }
    if (ploc.reg2 != kRinvalid) {
      CreateCallStructParamPassByReg(memOpnd, ploc.reg2, kThirdReg);
    }
    if (ploc.reg3 != kRinvalid) {
      CreateCallStructParamPassByReg(memOpnd, ploc.reg3, kFourthReg);
    }
  }
}

/*
 * SelectParmList generates an instrunction for each of the parameters
 * to load the parameter value into the corresponding register.
 * We return a list of registers to the call instruction because
 * they may be needed in the register allocation phase.
 */
void AArch64MPIsel::SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds) {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  aarch64CGFunc->SelectParmList(naryNode, srcOpnds);
}

bool AArch64MPIsel::IsParamStructCopy(const MIRSymbol &symbol) {
  if (symbol.GetStorageClass() == kScFormal &&
      cgFunc->GetBecommon().GetTypeSize(symbol.GetTyIdx().GetIdx()) > k16ByteSize) {
    return true;
  }
  return false;
}

bool AArch64MPIsel::IsSymbolRequireIndirection(const MIRSymbol &symbol) {
  return IsParamStructCopy(symbol);
}

void AArch64MPIsel::SelectIntAggCopyReturn(MemOperand &symbolMem, uint64 aggSize) {
  (void)symbolMem;
  (void)aggSize;
}

void AArch64MPIsel::SelectAggCopy(MemOperand &lhs, MemOperand &rhs, uint32 copySize) {
  (void)lhs;
  (void)rhs;
  (void)copySize;
  CHECK_FATAL_FALSE("Invalid MPISel function");
}

void AArch64MPIsel::SelectLibCallNoReturn(const std::string &funcName, std::vector<Operand*> &opndVec,
                                          PrimType primType) {
  /* generate libcall withou return value */
  std::vector <PrimType> pt(opndVec.size(), primType);
  SelectLibCallNArg(funcName, opndVec, pt);
  return;
}

void AArch64MPIsel::SelectLibCallNArg(const std::string &funcName, std::vector<Operand*> &opndVec,
                                      std::vector<PrimType> pt) {
  std::string newName = funcName;
  MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
  st->SetNameStrIdx(newName);
  st->SetStorageClass(kScExtern);
  st->SetSKind(kStFunc);

  /* setup the type of the callee function */
  std::vector<TyIdx> vec;
  std::vector<TypeAttrs> vecAt;
  for (size_t i = 1; i < opndVec.size(); ++i) {
    vec.emplace_back(GlobalTables::GetTypeTable().GetTypeTable()[static_cast<size_t>(pt[i])]->GetTypeIndex());
    vecAt.emplace_back(TypeAttrs());
  }

  /* only support no return function */
  MIRType *retType = GlobalTables::GetTypeTable().GetTypeTable().at(static_cast<size_t>(PTY_void));
  st->SetTyIdx(cgFunc->GetBecommon().BeGetOrCreateFunctionType(retType->GetTypeIndex(), vec, vecAt)->GetTypeIndex());

  /* setup actual parameters */
  ListOperand &paramOpnds = cgFunc->GetOpndBuilder()->CreateList();

  AArch64CallConvImpl parmLocator(cgFunc->GetBecommon());
  CCLocInfo ploc;
  for (size_t i = 0; i < opndVec.size(); ++i) {
    ASSERT(pt[i] != PTY_void, "primType check");
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<size_t>(pt[i])];
    Operand *stOpnd = opndVec[i];
    ASSERT(stOpnd->IsRegister(), "exp result should be reg");
    RegOperand *expRegOpnd = static_cast<RegOperand*>(stOpnd);
    parmLocator.LocateNextParm(*ty, ploc);
    if (ploc.reg0 != 0) {  /* load to the register */
      RegOperand &parmRegOpnd = cgFunc->GetOpndBuilder()->CreatePReg(ploc.reg0,
          expRegOpnd->GetSize(), cgFunc->GetRegTyFromPrimTy(pt[i]));
      SelectCopy(parmRegOpnd, *expRegOpnd, pt[i]);
      paramOpnds.PushOpnd(parmRegOpnd);
    }
    ASSERT(ploc.reg1 == 0, "SelectCall NYI");
  }

  MIRSymbol *sym = cgFunc->GetFunction().GetLocalOrGlobalSymbol(st->GetStIdx(), false);
  Operand &targetOpnd = cgFunc->GetOpndBuilder()->CreateFuncNameOpnd(*sym);
  ListOperand &retOpnds = cgFunc->GetOpndBuilder()->CreateList();
  Insn &callInsn = AppendCall(MOP_xbl, targetOpnd, paramOpnds, retOpnds);

  callInsn.SetRetType(Insn::kRegInt);
  if (retType != nullptr) {
    callInsn.SetRetSize(static_cast<uint32>(retType->GetSize()));
    callInsn.SetIsCallReturnUnsigned(IsUnsignedInteger(retType->GetPrimType()));
  }
  return;
}

RegOperand *AArch64MPIsel::PrepareMemcpyParm(MemOperand &memOperand, MOperator mOp) {
  RegOperand &regResult = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
  Insn &addrInsn = (cgFunc->GetInsnBuilder()->BuildInsn(mOp, AArch64CG::kMd[mOp]));
  addrInsn.AddOpndChain(memOperand).AddOpndChain(regResult);
  cgFunc->GetCurBB()->AppendInsn(addrInsn);
  return &regResult;
}

RegOperand *AArch64MPIsel::PrepareMemcpyParm(uint64 copySize) {
  RegOperand &regResult = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
  ImmOperand &sizeOpnd = cgFunc->GetOpndBuilder()->CreateImm(k64BitSize, static_cast<int64>(copySize));
  SelectCopy(regResult, sizeOpnd, PTY_i64);
  return &regResult;
}

void AArch64MPIsel::SelectAggDassign(MirTypeInfo &lhsInfo, MemOperand &symbolMem, Operand &opndRh,
    const DassignNode &stmt) {
  (void)lhsInfo;
  (void)symbolMem;
  (void)opndRh;
  cgFunc->SelectAggDassign(stmt);
}

void AArch64MPIsel::SelectAggIassign(IassignNode &stmt, Operand &addrOpnd, Operand &opndRhs) {
  (void)opndRhs;
  cgFunc->SelectAggIassign(stmt, addrOpnd);
}

Insn &AArch64MPIsel::AppendCall(AArch64MOP_t mOp, Operand &targetOpnd,
    ListOperand &paramOpnds, ListOperand &retOpnds) {
  Insn &callInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, AArch64CG::kMd[mOp]);
  callInsn.AddOpndChain(targetOpnd).AddOpndChain(paramOpnds).AddOpndChain(retOpnds);
  cgFunc->GetCurBB()->AppendInsn(callInsn);
  cgFunc->GetCurBB()->SetHasCall();
  cgFunc->GetFunction().SetHasCall();
  return callInsn;
}

void AArch64MPIsel::SelectCalleeReturn(MIRType *retType, ListOperand &retOpnds) {
  if (retType == nullptr) {
    return;
  }
  auto retSize = retType->GetSize() * kBitsPerByte;
  if (retType->GetPrimType() != PTY_agg || retSize <= k128BitSize) {
    if (retSize > k0BitSize) {
      retOpnds.PushOpnd(cgFunc->GetOpndBuilder()->CreatePReg(R0, k64BitSize, kRegTyInt));
    }
    if (retSize > k64BitSize) {
      retOpnds.PushOpnd(cgFunc->GetOpndBuilder()->CreatePReg(R1, k64BitSize, kRegTyInt));
    }
  }
}

void AArch64MPIsel::SelectCall(CallNode &callNode) {
  cgFunc->SelectCall(callNode);
}

void AArch64MPIsel::SelectIcall(IcallNode &iCallNode, Operand &opnd0) {
  cgFunc->SelectIcall(iCallNode, opnd0);
}

Operand &AArch64MPIsel::ProcessReturnReg(PrimType primType, int32 sReg) {
  return GetTargetRetOperand(primType, sReg);
}

void AArch64MPIsel::SelectGoto(GotoNode &stmt) {
  MOperator mOp = MOP_xuncond;
  auto funcName = ".L." + std::to_string(cgFunc->GetUniqueID()) + "__" + std::to_string(stmt.GetOffset());
  LabelOperand &targetOpnd = cgFunc->GetOpndBuilder()->CreateLabel(funcName.c_str(), stmt.GetOffset());
  Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, AArch64CG::kMd[mOp]);
  cgFunc->GetCurBB()->AppendInsn(jmpInsn);
  jmpInsn.AddOpndChain(targetOpnd);
  cgFunc->SetCurBBKind(BB::kBBGoto);
  return;
}

void AArch64MPIsel::SelectIgoto(Operand &opnd0) {
  CHECK_FATAL(opnd0.IsRegister(), "only register implemented!");
  MOperator mOp = MOP_xbr;
  Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, AArch64CG::kMd[mOp]);
  jmpInsn.AddOpndChain(opnd0);
  cgFunc->GetCurBB()->AppendInsn(jmpInsn);
  return;
}

/* The second parameter in function va_start does not need to be concerned here,
 * it is mainly used in proepilog */
void AArch64MPIsel::SelectCVaStart(const IntrinsiccallNode &intrnNode) {
  AArch64CGFunc *a64func = static_cast<AArch64CGFunc*>(cgFunc);
  a64func->SelectCVaStart(intrnNode);
}

void AArch64MPIsel::SelectIntrinCall(IntrinsiccallNode &intrinsiccallNode) {
  MIRIntrinsicID intrinsic = intrinsiccallNode.GetIntrinsic();

  if (intrinsic == INTRN_C_va_start) {
    SelectCVaStart(intrinsiccallNode);
    return;
  }
  if (intrinsic == INTRN_C_stack_save || intrinsic == INTRN_C_stack_restore) {
    return;
  }

  CHECK_FATAL_FALSE("Intrinsic %d: %s not implemented by AArch64 isel CG.", intrinsic, GetIntrinsicName(intrinsic));
}

void AArch64MPIsel::SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) {
  MIRType *etype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64));
  std::vector<uint32> sizeArray;
  const SmallCaseVector &switchTable = rangeGotoNode.GetRangeGotoTable();
  sizeArray.emplace_back(switchTable.size());
  MemPool *memPool = cgFunc->GetMemoryPool();
  MIRArrayType *arrayType = memPool->New<MIRArrayType>(etype->GetTypeIndex(), sizeArray);
  MIRAggConst *arrayConst = memPool->New<MIRAggConst>(cgFunc->GetMirModule(), *arrayType);
  for (const auto &itPair : switchTable) {
    LabelIdx labelIdx = itPair.second;
    cgFunc->GetCurBB()->PushBackRangeGotoLabel(labelIdx);
    MIRConst *mirConst = memPool->New<MIRLblConst>(labelIdx, cgFunc->GetFunction().GetPuidx(), *etype);
    arrayConst->AddItem(mirConst, 0);
  }
  MIRSymbol *lblSt = cgFunc->GetFunction().GetSymTab()->CreateSymbol(kScopeLocal);
  lblSt->SetStorageClass(kScFstatic);
  lblSt->SetSKind(kStConst);
  lblSt->SetTyIdx(arrayType->GetTypeIndex());
  lblSt->SetKonst(arrayConst);
  std::string lblStr(".L_");
  uint32 labelIdxTmp = cgFunc->GetLabelIdx();
  lblStr.append(std::to_string(cgFunc->GetUniqueID())).append("_LOCAL_CONST.").append(std::to_string(labelIdxTmp++));
  cgFunc->SetLabelIdx(labelIdxTmp);
  lblSt->SetNameStrIdx(lblStr);
  cgFunc->AddEmitSt(cgFunc->GetCurBB()->GetId(), *lblSt);

  ImmOperand &stOpnd = cgFunc->GetOpndBuilder()->CreateImm(*lblSt, 0, 0);
  /* get index */
  PrimType srcType = rangeGotoNode.Opnd(0)->GetPrimType();
  RegOperand &opnd0 = SelectCopy2Reg(srcOpnd, srcType);
  int32 minIdx = switchTable[0].first;
  ImmOperand &opnd1 = cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(srcType),
                                                          -minIdx - rangeGotoNode.GetTagOffset());
  RegOperand *indexOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(srcType), kRegTyInt);
  SelectAdd(*indexOpnd, opnd0, opnd1, srcType);
  if (indexOpnd->GetSize() != GetPrimTypeBitSize(PTY_u64)) {
    indexOpnd = static_cast<RegOperand*>(&cgFunc->SelectCopy(*indexOpnd, PTY_u64, PTY_u64));
  }

  /* load the address of the switch table */
  RegOperand &baseOpnd = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
  cgFunc->GetCurBB()->AppendInsn(cgFunc->GetInsnBuilder()->BuildInsn(MOP_xadrp, baseOpnd, stOpnd));
  cgFunc->GetCurBB()->AppendInsn(cgFunc->GetInsnBuilder()->BuildInsn(MOP_xadrpl12, baseOpnd, baseOpnd, stOpnd));

  /* load the displacement into a register by accessing memory at base + index*8 */
  AArch64CGFunc *a64func = static_cast<AArch64CGFunc*>(cgFunc);
  BitShiftOperand &bitOpnd = a64func->CreateBitShiftOperand(BitShiftOperand::kLSL, k3BitSize, k8BitShift);
  Operand *disp = static_cast<AArch64CGFunc*>(cgFunc)->CreateMemOperand(k64BitSize, baseOpnd, *indexOpnd, bitOpnd);
  RegOperand &tgt = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
  SelectAdd(tgt, baseOpnd, *disp, PTY_u64);
  Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_xbr, AArch64CG::kMd[MOP_xbr]);
  jmpInsn.AddOpndChain(tgt);
  cgFunc->GetCurBB()->AppendInsn(jmpInsn);
}

Operand *AArch64MPIsel::SelectAddrof(AddrofNode &expr, const BaseNode &parent) {
  return cgFunc->SelectAddrof(expr, parent, false);
}

Operand *AArch64MPIsel::SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) {
  return &cgFunc->SelectAddrofFunc(expr, parent);
}

Operand *AArch64MPIsel::SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) {
  (void)parent;
  /* adrp reg, label-id */
  uint32 instrSize = static_cast<uint32>(expr.SizeOfInstr());
  PrimType primType = (instrSize == k8ByteSize) ? PTY_u64 :
                      (instrSize == k4ByteSize) ? PTY_u32 :
                      (instrSize == k2ByteSize) ? PTY_u16 : PTY_u8;
  Operand &dst = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  ImmOperand &immOpnd = cgFunc->GetOpndBuilder()->CreateImm(k64BitSize, expr.GetOffset());
  cgFunc->GetCurBB()->AppendInsn(cgFunc->GetInsnBuilder()->BuildInsn(MOP_adrp_label, dst, immOpnd));
  return &dst;
}

/*
 * handle brfalse/brtrue op, opnd0 can be a compare node or non-compare node
 * such as a dread for example
 */
void AArch64MPIsel::SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode) {
  auto &condGotoNode = static_cast<CondGotoNode&>(stmt);
  Operand *opnd0 = nullptr;
  Operand *opnd1 = nullptr;
  if (!kOpcodeInfo.IsCompare(condNode.GetOpCode())) {
    Opcode condOp = condGotoNode.GetOpCode();
    if (condNode.GetOpCode() == OP_constval) {
      auto &constValNode = static_cast<ConstvalNode&>(condNode);
      if (((OP_brfalse == condOp) && constValNode.GetConstVal()->IsZero()) ||
          ((OP_brtrue == condOp) && !constValNode.GetConstVal()->IsZero())) {
        auto *gotoStmt = cgFunc->GetMemoryPool()->New<GotoNode>(OP_goto);
        gotoStmt->SetOffset(condGotoNode.GetOffset());
        HandleGoto(*gotoStmt, *this);            // isel's
        auto *labelStmt = cgFunc->GetMemoryPool()->New<LabelNode>();
        labelStmt->SetLabelIdx(cgFunc->CreateLabel());
        HandleLabel(*labelStmt, *this);
      }
      return;
    }
    /* 1 operand condNode, cmp it with zero */
    opnd0 = HandleExpr(stmt, condNode);          // isel's
    opnd1 = &cgFunc->CreateImmOperand(condNode.GetPrimType(), 0);
  } else {
    /* 2 operands condNode */
    opnd0 = HandleExpr(stmt, *condNode.Opnd(0)); // isel's
    opnd1 = HandleExpr(stmt, *condNode.Opnd(1)); // isel's
  }
  cgFunc->SelectCondGoto(stmt, *opnd0, *opnd1);
  cgFunc->SetCurBBKind(BB::kBBIf);
}

Operand *AArch64MPIsel::SelectStrLiteral(ConststrNode &constStr) {
  return cgFunc->SelectStrConst(*cgFunc->GetMemoryPool()->New<MIRStrConst>(
    constStr.GetStrIdx(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64))));
}

Operand &AArch64MPIsel::GetTargetRetOperand(PrimType primType, int32 sReg) {
  regno_t retReg = 0;
  switch (sReg) {
    case kSregRetval0:
      if (IsPrimitiveFloat(primType)) {
        retReg = V0;
      } else {
        retReg = R0;
      }
      break;
    case kSregRetval1:
      if (IsPrimitiveFloat(primType)) {
        retReg = V1;
      } else {
        retReg = R1;
      }
      break;
    default:
      CHECK_FATAL_FALSE("GetTargetRetOperand: NIY");
      break;
  }
  uint32 bitSize = GetPrimTypeBitSize(primType);
  RegOperand &parmRegOpnd = cgFunc->GetOpndBuilder()->CreatePReg(retReg, bitSize,
      cgFunc->GetRegTyFromPrimTy(primType));
  return parmRegOpnd;
}

Operand *AArch64MPIsel::SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  (void)parent;
  PrimType dtype = node.GetPrimType();
  RegOperand *resOpnd = nullptr;
  if (!IsPrimitiveVector(dtype)) {
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype),
        cgFunc->GetRegTyFromPrimTy(dtype));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, dtype, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, dtype, node.Opnd(1)->GetPrimType());
    SelectMpy(*resOpnd, regOpnd0, regOpnd1, dtype);
  } else {
    /* vector operand */
    CHECK_FATAL_FALSE("NIY");
  }

  return resOpnd;
}

void AArch64MPIsel::SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  cgFunc->SelectMpy(resOpnd, opnd0, opnd1, primType);
}

Operand *AArch64MPIsel::SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  return cgFunc->SelectDiv(node, opnd0, opnd1, parent);
}

Operand *AArch64MPIsel::SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  return cgFunc->SelectRem(node, opnd0, opnd1, parent);
}

Operand *AArch64MPIsel::SelectDivRem(RegOperand &opnd0, RegOperand &opnd1, PrimType primType, Opcode opcode) {
  (void)opnd0;
  (void)opnd1;
  (void)primType;
  (void)opcode;
  CHECK_FATAL_FALSE("Invalid MPISel function");
  return nullptr;
}

Operand *AArch64MPIsel::SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  return cgFunc->SelectCmpOp(node, opnd0, opnd1, parent);
}

void AArch64MPIsel::SelectCmp(Operand &opnd0, Operand &opnd1, PrimType primType) {
  (void)opnd0;
  (void)opnd1;
  (void)primType;
  CHECK_FATAL_FALSE("Invalid MPISel function");
}

Operand *AArch64MPIsel::SelectSelect(TernaryNode &expr, Operand &cond, Operand &trueOpnd, Operand &falseOpnd,
                                     const BaseNode &parent) {
  return cgFunc->SelectSelect(expr, cond, trueOpnd, falseOpnd, parent);
}

Operand *AArch64MPIsel::SelectExtractbits(const BaseNode &parent, ExtractbitsNode &node, Operand &opnd0) {
  return cgFunc->SelectExtractbits(node, opnd0, parent);
}

void AArch64MPIsel::SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  AArch64CGFunc *a64func = static_cast<AArch64CGFunc*>(cgFunc);
  a64func->SelectMinOrMax(isMin, resOpnd, opnd0, opnd1, primType);
}

Operand *AArch64MPIsel::SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrnNode, std::string name, Operand &opnd0,
    const BaseNode &parent) {
  PrimType ptype = intrnNode.Opnd(0)->GetPrimType();
  Operand *opnd = &opnd0;
  AArch64CGFunc *a64func = static_cast<AArch64CGFunc*>(cgFunc);
  if (intrnNode.GetIntrinsic() == INTRN_C_ffs) {
    ASSERT(intrnNode.GetPrimType() == PTY_i32, "Unexpect Size");
    return a64func->SelectAArch64ffs(*opnd, ptype);
  }
  if (opnd->IsMemoryAccessOperand()) {
    RegOperand &ldDest = a64func->CreateRegisterOperandOfType(ptype);
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(a64func->PickLdInsn(GetPrimTypeBitSize(ptype), ptype),
        ldDest, *opnd);
    cgFunc->GetCurBB()->AppendInsn(insn);
    opnd = &ldDest;
  }
  std::vector<Operand *> opndVec;
  RegOperand *dst = &a64func->CreateRegisterOperandOfType(ptype);
  opndVec.push_back(dst);  /* result */
  opndVec.push_back(opnd); /* param 0 */
  a64func->SelectLibCall(name, opndVec, ptype, ptype);

  return dst;
}

Operand *AArch64MPIsel::SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return cgFunc->SelectBswap(node, opnd0, parent);
}

Operand *AArch64MPIsel::SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return cgFunc->SelectCctz(node);
}

Operand *AArch64MPIsel::SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return cgFunc->SelectCclz(node);
}

Operand *AArch64MPIsel::SelectCsin(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "sin", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCsinh(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "sinh", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCasin(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "asin", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCcos(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "cos", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCcosh(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "cosh", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCacos(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "acos", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCatan(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "atan", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCexp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "exp", opnd0, parent);
}

Operand *AArch64MPIsel::SelectClog(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "log", opnd0, parent);
}

Operand *AArch64MPIsel::SelectClog10(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "log10", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCsinf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "sinf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCsinhf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "sinhf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCasinf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "asinf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCcosf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "cosf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCcoshf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "coshf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCacosf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "acosf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCatanf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "atanf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCexpf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "expf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectClogf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "logf", opnd0, parent);
}

Operand *AArch64MPIsel::SelectClog10f(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "log10f", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCffs(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  return SelectIntrinsicOpWithOneParam(node, "ffs", opnd0, parent);
}

Operand *AArch64MPIsel::SelectCmemcmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL_FALSE("NYI");
  return nullptr;
}

Operand *AArch64MPIsel::SelectCstrlen(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL_FALSE("NYI");
  return nullptr;
}

Operand *AArch64MPIsel::SelectCstrcmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL_FALSE("NYI");
  return nullptr;
}

Operand *AArch64MPIsel::SelectCstrncmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL_FALSE("NYI");
  return nullptr;
}

Operand *AArch64MPIsel::SelectCstrchr(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL_FALSE("NYI");
  return nullptr;
}

Operand *AArch64MPIsel::SelectCstrrchr(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL_FALSE("NYI");
  return nullptr;
}

Operand *AArch64MPIsel::SelectAbs(UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  return cgFunc->SelectAbs(node, opnd0);
}

void AArch64MPIsel::SelectCvtFloat2Float(Operand &resOpnd, Operand &srcOpnd, PrimType fromType, PrimType toType) {
   static_cast<AArch64CGFunc*>(cgFunc)->SelectCvtFloat2Float(resOpnd, srcOpnd, fromType, toType);
}

void AArch64MPIsel::SelectCvtFloat2Int(Operand &resOpnd, Operand &srcOpnd, PrimType itype, PrimType ftype) {
   static_cast<AArch64CGFunc*>(cgFunc)->SelectCvtFloat2Int(resOpnd, srcOpnd, itype, ftype);
}

RegOperand &AArch64MPIsel::GetTargetStackPointer(PrimType primType) {
  return cgFunc->GetOpndBuilder()->CreatePReg(RSP, GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
}

RegOperand &AArch64MPIsel::GetTargetBasicPointer(PrimType primType) {
  return cgFunc->GetOpndBuilder()->CreatePReg(RFP, GetPrimTypeBitSize(primType),
      cgFunc->GetRegTyFromPrimTy(primType));
}

void AArch64MPIsel::SelectAsm(AsmNode &node) {
  cgFunc->SelectAsm(node);
}
}
