/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <sstream>
#include "driver_options.h"
#include "global_tables.h"
#include "inline.h"
#include "me_dominance.h"
#include "me_irmap_build.h"
#include "me_option.h"
#include "mpl_logging.h"
#include "opcode_info.h"

namespace maple {
inline static bool HandleAssertNonnull(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertNonnullMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN_USER(kLncWarn, srcPosition, mod, "Dereference of nullable pointer");
  } else {
    WARN_USER(kLncWarn, srcPosition, mod, "Dereference of nullable pointer when inlined to %s",
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
      WARN_USER(kLncWarn, srcPosition, mod, "%s return nonnull but got nullable pointer",
          returnStmt.GetFuncName().c_str());
    }
  } else {
    if (MeOption::safeRegionMode && stmt.IsInSafeRegion()) {
      FATAL(kLncFatal, "%s:%d error: %s return nonnull but got nullable pointer in safe region when inlined to %s",
            mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(),
            returnStmt.GetFuncName().c_str(), func.GetName().c_str());
    } else {
      WARN_USER(kLncWarn, srcPosition, mod, "%s return nonnull but got nullable pointer when inlined to %s",
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
      WARN_USER(kLncWarn, srcPosition, mod, "nullable pointer assignment of nonnull pointer");
    }
  } else {
    if (MeOption::safeRegionMode && stmt.IsInSafeRegion()) {
      FATAL(kLncFatal, "%s:%d error: nullable pointer assignment of nonnull pointer in safe region when inlined to %s",
            mod.GetFileNameFromFileNum(srcPosition.FileNum()).c_str(), srcPosition.LineNum(), func.GetName().c_str());
    } else {
      WARN_USER(kLncWarn, srcPosition, mod, "nullable pointer assignment of nonnull pointer when inlined to %s",
          func.GetName().c_str());
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
    WARN_USER(kLncWarn, srcPosition, mod, "nullable pointer passed to %s that requires nonnull for %s argument",
              callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
  } else {
    WARN_USER(kLncWarn, srcPosition, mod,
              "nullable pointer passed to %s that requires nonnull for %s argument when inlined to %s",
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
      oss << "can't prove the pointer < the upper bounds after calculation";
    } else {
      oss << "can't prove the pointer >= the lower bounds after calculation";
    }
    WARN_USER(kLncWarn, srcPosition, mod, oss.str().c_str());
  } else {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "can't prove the pointer < the upper bounds after calculation when inlined to %s";
    } else {
      oss << "can't prove the pointer >= the lower bounds after calculation when inlined to %s";
    }
    WARN_USER(kLncWarn, srcPosition, mod, oss.str().c_str(), func.GetName().c_str());
  }
  return !opts::enableArithCheck || MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
}

static bool HandleMemoryAccessAssert(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  std::ostringstream oss;
  if (curFuncNameIdx == stmtFuncNameIdx) {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "can't prove the pointer < the upper bounds when accessing the memory";
    } else {
      oss << "can't prove the pointer >= the lower bounds when accessing the memory";
    }
    WARN_USER(kLncWarn, srcPosition, mod, oss.str().c_str());
  } else {
    if (kOpcodeInfo.IsAssertUpperBoundary(stmt.GetOp())) {
      oss << "can't prove the pointer < the upper bounds when accessing the memory and inlined to %s";
    } else {
      oss << "can't prove the pointer >= the lower bounds when accessing the memory and inlined to %s";
    }
    WARN_USER(kLncWarn, srcPosition, mod, oss.str().c_str(), func.GetName().c_str());
  }
  return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck || opts::enableArithCheck;
}

static bool HandleBoundaryCheckAssertAssign(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &newStmt = static_cast<const AssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(newStmt.GetFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN_USER(kLncWarn, srcPosition, mod, "can't prove l-value's upper bounds <= r-value's upper bounds");
  } else {
    WARN_USER(kLncWarn, srcPosition, mod,
        "can't prove l-value's upper bounds <= r-value's upper bounds when inlined to %s",
        func.GetName().c_str());
  }

  return MeOption::boundaryCheckMode == SafetyCheckMode::kStaticCheck;
}

inline static bool HandleBoundaryCheckAssertCall(const MeStmt &stmt, const MIRModule &mod, const MeFunction &func) {
  auto srcPosition = stmt.GetSrcPosition();
  auto &callStmt = static_cast<const CallAssertBoundaryMeStmt &>(stmt);
  GStrIdx curFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(func.GetName().c_str());
  GStrIdx stmtFuncNameIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callStmt.GetStmtFuncName().c_str());
  if (curFuncNameIdx == stmtFuncNameIdx) {
    WARN_USER(kLncWarn, srcPosition, mod,
              "can't prove pointer's bounds match the function %s declaration for the %s argument",
              callStmt.GetFuncName().c_str(), GetNthStr(callStmt.GetParamIndex()).c_str());
  } else {
    WARN_USER(kLncWarn, srcPosition, mod,
              "can't prove pointer's bounds match the function %s declaration for the %s argument when inlined to %s",
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
    WARN_USER(kLncWarn, srcPosition, mod, "can't prove return value's bounds match the function declaration for %s",
              returnStmt.GetFuncName().c_str());
  } else {
    WARN_USER(kLncWarn, srcPosition, mod,
              "can't prove return value's bounds match the function declaration for %s when inlined to %s",
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
    { OP_assertlt, [](auto&, auto&, auto&) -> bool { return opts::enableArithCheck; } },
    { OP_assertge, [](auto&, auto&, auto&) -> bool { return opts::enableArithCheck; } },
    { OP_calcassertlt, [](auto&, auto&, auto&) -> bool { return !opts::enableArithCheck; } },
    { OP_calcassertge, [](auto&, auto&, auto&) -> bool { return !opts::enableArithCheck; } },
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
  }
}

SafetyWarningHandler *MESafetyWarning::FindHandler(Opcode op) const {
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
  auto &mod = meFunction.GetMIRModule();
  MapleVector<MeStmt*> removeStmts(mod.GetMPAllocator().Adapter());
  for (auto *bb : meFunction.GetCfg()->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    for (auto &stmt : bb->GetMeStmts()) {
      auto *handle = FindHandler(stmt.GetOp());
      if (handle == nullptr) {
        continue;
      }
      // check has warned or not
      if (mod.HasNotWarned(stmt.GetSrcPosition().LineNum(), stmt.GetOriginalId())) {
        if ((*handle)(stmt, mod, meFunction)) {
          // record stmt to delete
          removeStmts.emplace_back(&stmt);
        }
      } else if (IsStaticModeForOp(stmt.GetOp()) ||
                 // deref assert nonnull need to remove in all mode
                 stmt.GetOp() == OP_assertnonnull ||
                 (!opts::enableArithCheck && kOpcodeInfo.IsCalcAssertBoundary(stmt.GetOp())) ||
                 (opts::enableArithCheck && kOpcodeInfo.IsAccessAssertBoundary(stmt.GetOp()))) {
        // remove inlined code
        removeStmts.emplace_back(&stmt);
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
