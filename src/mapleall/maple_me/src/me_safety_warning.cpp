/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_safety_warning.h"
#include "global_tables.h"
#include "inline.h"
#include "me_dominance.h"
#include "me_irmap_build.h"
#include "me_option.h"
#include "mpl_logging.h"
#include "opcode_info.h"
#include <sstream>

namespace maple {
std::string GetNthStr(size_t index) {
  switch (index) {
    case 0:
      return "1st";
    case 1:
      return "2nd";
    case 2:
      return "3rd";
    default: {
      std::ostringstream oss;
      oss << index + 1 << "th";
      return oss.str();
    }
  }
}

inline static bool HandleAssertNonnull(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertNonnullMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN(kLncWarn, "%s:%d warning: Dereference of nullable pointer",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
  } else {
    WARN(kLncWarn, "%s:%d warning: Dereference of nullable pointer when inlined to %s",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         newStmt.GetFuncName().c_str());
  }
  return !MeOption::isNpeCheckAll || MeOption::npeCheckMode == SafetyCheckMode::kStaticCheck;
}

inline static bool HandleReturnAssertNonnull(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &returnStmt = static_cast<const AssertNonnullMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(returnStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    if (MeOption::safeRegionMode && stmt.IsInSafeRegion()) {
      FATAL(kLncFatal, "%s:%d error: %s return nonnull but got nullable pointer in safe region",
            mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            returnStmt.GetFuncName().c_str());
    } else {
      WARN(kLncWarn, "%s:%d warning: %s return nonnull but got nullable pointer",
           mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
           returnStmt.GetFuncName().c_str());
    }
  } else {
    if (MeOption::safeRegionMode && stmt.IsInSafeRegion()) {
      FATAL(kLncFatal, "%s:%d error: %s return nonnull but got nullable pointer in safe region when inlined to %s",
            mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            returnStmt.GetFuncName().c_str(), func.GetName().c_str());
    } else {
      WARN(kLncWarn, "%s:%d warning: %s return nonnull but got nullable pointer when inlined to %s",
           mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
           returnStmt.GetFuncName().c_str(), func.GetName().c_str());
    }
  }

  return MeOption::npeCheckMode == SafetyCheckMode::kStaticCheck;
}

inline static bool HandleAssignAssertNonnull(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &assignStmt = static_cast<const AssertNonnullMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(assignStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    if (MeOption::safeRegionMode && stmt.IsInSafeRegion()) {
      FATAL(kLncFatal, "%s:%d error: nullable pointer assignment of nonnull pointer in safe region",
            mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
    } else {
      WARN(kLncWarn, "%s:%d warning: nullable pointer assignment of nonnull pointer",
           mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
    }
  } else {
    if (MeOption::safeRegionMode && stmt.IsInSafeRegion()) {
      FATAL(kLncFatal, "%s:%d error: nullable pointer assignment of nonnull pointer in safe region when inlined to %s",
            mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(), func.GetName().c_str());
    } else {
      WARN(kLncWarn, "%s:%d warning: nullable pointer assignment of nonnull pointer when inlined to %s",
           mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(), func.GetName().c_str());
    }
  }
  return MeOption::npeCheckMode == SafetyCheckMode::kStaticCheck;
}

inline static bool HandleCallAssertNonnull(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &callStmt = static_cast<const CallAssertNonnullMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callStmt.GetStmtFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN(kLncWarn, "%s:%d warning: nullable pointer passed to %s that requires nonnull for %s argument",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
  } else {
    WARN(kLncWarn,
         "%s:%d warning: nullable pointer passed to %s that requires nonnull for %s argument when inlined to %s",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str(), func.GetName().c_str());
  }
  return MeOption::npeCheckMode == SafetyCheckMode::kStaticCheck;
}

static bool HandleCalculationAssert(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  std::ostringstream oss;
  if (curFuncNameIdx == stmtFuncNameIdx) {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "%s:%d warning: can't prove the pointer < the upper bounds after calculation";
    } else {
      oss << "%s:%d warning: can't prove the pointer >= the lower bounds after calculation";
    }
    WARN(kLncWarn, oss.str().c_str(), mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
  } else {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "%s:%d warning: can't prove the pointer < the upper bounds after calculation when inlined to %s";
    } else {
      oss << "%s:%d warning: can't prove the pointer >= the lower bounds after calculation when inlined to %s";
    }
    WARN(kLncWarn, oss.str().c_str(), mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         func.GetName().c_str());
  }
  return true;
}

static bool HandleMemoryAccessAssert(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  std::ostringstream oss;
  if (curFuncNameIdx == stmtFuncNameIdx) {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "%s:%d warning: can't prove the pointer < the upper bounds when accessing the memory";
    } else {
      oss << "%s:%d warning: can't prove the pointer >= the lower bounds when accessing the memory";
    }
    WARN(kLncWarn, oss.str().c_str(), mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
  } else {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "%s:%d warning: can't prove the pointer < the upper bounds when accessing the memory and inlined to %s";
    } else {
      oss << "%s:%d warning: can't prove the pointer >= the lower bounds when accessing the memory and inlined to %s";
    }
    WARN(kLncWarn, oss.str().c_str(), mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         func.GetName().c_str());
  }
  return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
}

static bool HandleBoundaryCheckAssertAssign(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN(kLncWarn, "%s:%d warning: can't prove l-value's upper bounds <= r-value's upper bounds",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum());
  } else {
    WARN(kLncWarn, "%s:%d warning: can't prove l-value's upper bounds <= r-value's upper bounds when inlined to %s",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(), func.GetName().c_str());
  }

  return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
}

inline static bool HandleBoundaryCheckAssertCall(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &callStmt = static_cast<const CallAssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callStmt.GetStmtFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN(kLncWarn, "%s:%d warning: can't prove pointer's bounds match the function %s declaration for the %s argument",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
  } else {
    WARN(kLncWarn, "%s:%d warning: can't prove pointer's bounds match the function %s declaration for the %s argument "\
         "when inlined to %s", mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str(), func.GetName().c_str());
  }
  return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
}

inline static bool HandleBoundaryCheckAssertReturn(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &returnStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(returnStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN(kLncWarn, "%s:%d warning: can't prove return value's bounds match the function declaration for %s",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         returnStmt.GetFuncName().c_str());
  } else {
    WARN(kLncWarn,
         "%s:%d warning: can't prove return value's bounds match the function declaration for %s when inlined to %s",
         mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
         returnStmt.GetFuncName().c_str(), func.GetName().c_str());
  }
  return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
}

SafetyWarningHandlers MESafetyWarning::npeHandleMap = {
    { OP_assertnonnull, HandleAssertNonnull },
    { OP_returnassertnonnull, HandleReturnAssertNonnull },
    { OP_assignassertnonnull, HandleAssignAssertNonnull },
    { OP_callassertnonnull, HandleCallAssertNonnull }
};

SafetyWarningHandlers MESafetyWarning::boundaryHandleMap = {
    { OP_assertlt, HandleMemoryAccessAssert },
    { OP_assertge, HandleMemoryAccessAssert },
    { OP_calcassertlt, HandleCalculationAssert },
    { OP_calcassertge, HandleCalculationAssert },
    { OP_callassertle, HandleBoundaryCheckAssertCall },
    { OP_returnassertle, HandleBoundaryCheckAssertReturn },
    { OP_assignassertle, HandleBoundaryCheckAssertAssign }
};

SafetyWarningHandlers MESafetyWarning::npeSilentHandleMap = {
    { OP_assertnonnull, [](auto&, auto&, auto&) -> bool { return !MeOption::isNpeCheckAll; } },
    { OP_returnassertnonnull, [](auto&, auto&, auto&) -> bool { return false; } },
    { OP_assignassertnonnull, [](auto&, auto&, auto&) -> bool { return false; } },
    { OP_callassertnonnull, [](auto&, auto&, auto&) -> bool { return false; } }
};

SafetyWarningHandlers MESafetyWarning::boundarySilentHandleMap = {
    { OP_assertlt, [](auto&, auto&, auto&) -> bool { return false; } },
    { OP_assertge, [](auto&, auto&, auto&) -> bool { return false; } },
    { OP_calcassertlt, [](auto&, auto&, auto&) -> bool { return true; } },
    { OP_calcassertge, [](auto&, auto&, auto&) -> bool { return true; } },
    { OP_callassertle, [](auto&, auto&, auto&) -> bool { return false; } },
    { OP_returnassertle, [](auto&, auto&, auto&) -> bool { return false; } },
    { OP_assignassertle, [](auto&, auto&, auto&) -> bool { return false; } }
};

void MESafetyWarning::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MESafetyWarning::IsStaticModeForOp(Opcode op) const {
  switch (op) {
    CASE_OP_ASSERT_NONNULL
    return MeOption::npeCheckMode == SafetyCheckMode::kStaticCheck;
    CASE_OP_ASSERT_BOUNDARY
    return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
    default:
      CHECK_FATAL(false, "NEVER REACH");
      break;
  }
  return false;
}

SafetyWarningHandler *MESafetyWarning::FindHandler(Opcode op) {
  auto handler = realNpeHandleMap->find(op);
  if (handler != realNpeHandleMap->end()) {
    return &handler->second;
  }
  handler = realBoundaryHandleMap->find(op);
  if (handler != realBoundaryHandleMap->end()) {
    return &handler->second;
  }
  return nullptr;
}

bool MESafetyWarning::PhaseRun(MeFunction &meFunction) {
  auto *dom = GET_ANALYSIS(MEDominance, meFunction);
  auto &mod = meFunction.GetMIRModule();
  MapleVector<MeStmt*> removeStmts(mod.GetMPAllocator().Adapter());
  for (auto *bb : dom->GetReversePostOrder()) {
    for (auto &stmt : bb->GetMeStmts()) {
      auto *handle = FindHandler(stmt.GetOp());
      if (handle != nullptr) {
        // check has warned or not
        if (mod.HasNotWarned(stmt.GetSrcPosition().LineNum(), stmt.GetOriginalId())) {
          if ((*handle)(stmt, mod, meFunction)) {
            // record stmt to delete
            removeStmts.emplace_back(&stmt);
          }
        } else if (IsStaticModeForOp(stmt.GetOp()) ||
                   // deref assert nonnull need to remove in all mode
                   stmt.GetOp() == OP_assertnonnull ||
                   // calcassert need to be removed in all mode
                   kOpcodeInfo.IsCalcAssertBoundary(stmt.GetOp())) {
          // remove inlined code
          removeStmts.emplace_back(&stmt);
        }
      }
    }
    // remove stmt
    for (auto &stmt : removeStmts) {
      bb->RemoveMeStmt(stmt);
    }
    removeStmts.clear();
  }
  return true;
}
}  // namespace maple
