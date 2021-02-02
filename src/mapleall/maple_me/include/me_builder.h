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
#ifndef MAPLE_ME_INCLUDE_MEBUILDER_H
#define MAPLE_ME_INCLUDE_MEBUILDER_H
#include "me_ir.h"

namespace maple {
class MeBuilder {
 public:
  explicit MeBuilder(MapleAllocator &allocator) : allocator(allocator) {
    (void)InitMeExprBuildFactory();
  }

  virtual ~MeBuilder() = default;

  MeExpr &CreateMeExpr(int32 exprId, MeExpr &meExpr) const;
  MeExpr *BuildMeExpr(BaseNode &mirNode) const;
  VarMeExpr *BuildVarMeExpr(int32 exprID, OStIdx oStIdx, size_t vStIdx, PrimType pType, FieldID fieldID) const;

  UnaryMeStmt &BuildUnaryMeStmt(Opcode op, MeExpr &opnd, BB &bb, const SrcPosition &src) const;
  UnaryMeStmt &BuildUnaryMeStmt(Opcode op, MeExpr &opnd, BB &bb) const;
  UnaryMeStmt &BuildUnaryMeStmt(Opcode op, MeExpr &opnd) const;

 private:
  template<class T, typename... Arguments>
  T *NewInPool(Arguments&&... args) const {
    return allocator.GetMemPool()->New<T>(&allocator, std::forward<Arguments>(args)...);
  }

  template<class T, typename... Arguments>
  T *New(Arguments&&... args) const {
    return allocator.GetMemPool()->New<T>(std::forward<Arguments>(args)...);
  }

  OpMeExpr *BuildOpMeExpr(BaseNode &mirNode) const {
    OpMeExpr *meExpr = New<OpMeExpr>(kInvalidExprID, mirNode.GetOpCode(), mirNode.GetPrimType(), mirNode.GetNumOpnds());
    return meExpr;
  }

  static bool InitMeExprBuildFactory();
  MeExpr *BuildAddrofMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildAddroffuncMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildGCMallocMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildSizeoftypeMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildFieldsDistMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildIvarMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConstMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConststrMeExpr(BaseNode &mirNode) const;
  MeExpr *BuildConststr16MeExpr(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForCompare(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForTypeCvt(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForRetype(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForIread(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForExtractbits(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForJarrayMalloc(BaseNode &mirNode) const;
  MeExpr *BuildOpMeExprForResolveFunc(BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForArray(BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForIntrinsicop(BaseNode &mirNode) const;
  MeExpr *BuildNaryMeExprForIntrinsicWithType(BaseNode &mirNode) const;

  MapleAllocator &allocator;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEBUILDER_H
