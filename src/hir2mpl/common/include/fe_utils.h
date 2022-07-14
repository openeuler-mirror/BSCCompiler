/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_FE_UTILS_H
#define HIR2MPL_INCLUDE_FE_UTILS_H
#include <vector>
#include <string>
#include <list>
#include "mpl_logging.h"
#include "prim_types.h"
#include "mir_type.h"
#include "global_tables.h"
#include "mempool.h"
#include "mir_nodes.h"

namespace maple {
struct Loc {
  uint32 fileIdx;
  uint32 line;
  uint32 column;
  Loc(uint32 fileIdxIn, uint32 lineIn, uint32 columnIn) : fileIdx(fileIdxIn),  line(lineIn), column(columnIn) {}

  SrcPosition Emit2SourcePosition() const {
    SrcPosition srcPos;
    srcPos.SetFileNum(static_cast<uint16>(fileIdx));
    srcPos.SetLineNum(line);
    srcPos.SetColumn(static_cast<uint16>(column));
    return srcPos;
  }

  bool operator < (Loc const &loc) const {
    if (fileIdx != loc.fileIdx) {
      return fileIdx < loc.fileIdx;
    } else if (line != loc.line) {
      return line < loc.line;
    } else {
      return column < loc.column;
    }
  }
};

class FEUtils {
 public:
  FEUtils() = default;
  ~FEUtils() = default;
  static std::vector<std::string> Split(const std::string &str, char delim);
  static uint8 GetWidth(PrimType primType);
  static bool IsInteger(PrimType primType);
  static bool IsSignedInteger(PrimType primType);
  static bool IsUnsignedInteger(PrimType primType);
  static PrimType MergePrimType(PrimType primType1, PrimType primType2);
  static uint8 GetDim(const std::string &typeName);
  static std::string GetBaseTypeName(const std::string &typeName);
  static PrimType GetPrimType(const GStrIdx &typeNameIdx);
  static uint32 GetSequentialNumber();
  static std::string GetFileNameHashStr(const std::string &fileName, uint32 seed = 211);
  static std::string GetSequentialName0(const std::string &prefix, uint32_t num);
  static std::string GetSequentialName(const std::string &prefix);
  static std::string CreateLabelName();
  static FieldID GetStructFieldID(MIRStructType *base, const std::string &fieldName);
  static bool TraverseToNamedField(MIRStructType &structType, const GStrIdx &nameIdx, FieldID &fieldID,
                                   bool isTopLevel = true);
  static MIRType *GetStructFieldType(const MIRStructType *type, FieldID feildID);
  static const MIRFuncType *GetFuncPtrType(const MIRType &type);
  static MIRConst *CreateImplicitConst(MIRType *type);
  static PrimType GetVectorElementPrimType(PrimType vectorPrimType);
  static bool EndsWith(const std::string &value, const std::string &ending);
  static MIRConst *TraverseToMIRConst(MIRAggConst *aggConst, const MIRStructType &structType, FieldID &fieldID);
  static Loc GetSrcLocationForMIRSymbol(const MIRSymbol &symbol);

  static const std::string kBoolean;
  static const std::string kByte;
  static const std::string kShort;
  static const std::string kChar;
  static const std::string kInt;
  static const std::string kLong;
  static const std::string kFloat;
  static const std::string kDouble;
  static const std::string kVoid;
  static const std::string kThis;
  static const std::string kMCCStaticFieldGetBool;
  static const std::string kMCCStaticFieldGetByte;
  static const std::string kMCCStaticFieldGetChar;
  static const std::string kMCCStaticFieldGetShort;
  static const std::string kMCCStaticFieldGetInt;
  static const std::string kMCCStaticFieldGetLong;
  static const std::string kMCCStaticFieldGetFloat;
  static const std::string kMCCStaticFieldGetDouble;
  static const std::string kMCCStaticFieldGetObject;

  static const std::string kMCCStaticFieldSetBool;
  static const std::string kMCCStaticFieldSetByte;
  static const std::string kMCCStaticFieldSetChar;
  static const std::string kMCCStaticFieldSetShort;
  static const std::string kMCCStaticFieldSetInt;
  static const std::string kMCCStaticFieldSetLong;
  static const std::string kMCCStaticFieldSetFloat;
  static const std::string kMCCStaticFieldSetDouble;
  static const std::string kMCCStaticFieldSetObject;

  static inline MemPool *NewMempool(const std::string &name, bool isLocalPool) {
    return memPoolCtrler.NewMemPool(name, isLocalPool);
  }

  static inline void DeleteMempoolPtr(MemPool *memPoolPtr) {
    memPoolCtrler.DeleteMemPool(memPoolPtr);
  }

  static inline GStrIdx &GetBooleanIdx() {
    static GStrIdx booleanIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kBoolean);
    return booleanIdx;
  }

  static inline GStrIdx &GetByteIdx() {
    static GStrIdx byteIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kByte);
    return byteIdx;
  }

  static inline GStrIdx &GetShortIdx() {
    static GStrIdx shortIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kShort);
    return shortIdx;
  }

  static inline GStrIdx &GetCharIdx() {
    static GStrIdx charIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kChar);
    return charIdx;
  }

  static inline GStrIdx &GetIntIdx() {
    static GStrIdx intIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kInt);
    return intIdx;
  }

  static inline GStrIdx &GetLongIdx() {
    static GStrIdx longIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kLong);
    return longIdx;
  }

  static inline GStrIdx &GetFloatIdx() {
    static GStrIdx floatIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kFloat);
    return floatIdx;
  }

  static inline GStrIdx &GetDoubleIdx() {
    static GStrIdx doubleIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kDouble);
    return doubleIdx;
  }

  static inline GStrIdx &GetVoidIdx() {
    static GStrIdx voidIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kVoid);
    return voidIdx;
  }

  static inline GStrIdx &GetThisIdx() {
    static GStrIdx thisIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kThis);
    return thisIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetBoolIdx() {
    static GStrIdx mccStaticFieldGetBoolIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetBool);
    return mccStaticFieldGetBoolIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetByteIdx() {
    static GStrIdx mccStaticFieldGetByteIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetByte);
    return mccStaticFieldGetByteIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetShortIdx() {
    static GStrIdx mccStaticFieldGetShortIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetShort);
    return mccStaticFieldGetShortIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetCharIdx() {
    static GStrIdx mccStaticFieldGetCharIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetChar);
    return mccStaticFieldGetCharIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetIntIdx() {
    static GStrIdx mccStaticFieldGetIntIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetInt);
    return mccStaticFieldGetIntIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetLongIdx() {
    static GStrIdx mccStaticFieldGetLongIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetLong);
    return mccStaticFieldGetLongIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetFloatIdx() {
    static GStrIdx mccStaticFieldGetFloatIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetFloat);
    return mccStaticFieldGetFloatIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetDoubleIdx() {
    static GStrIdx mccStaticFieldGetDoubleIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetDouble);
    return mccStaticFieldGetDoubleIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldGetObjectIdx() {
    static GStrIdx mccStaticFieldGetObjectIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldGetObject);
    return mccStaticFieldGetObjectIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetBoolIdx() {
    static GStrIdx mccStaticFieldSetBoolIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetBool);
    return mccStaticFieldSetBoolIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetByteIdx() {
    static GStrIdx mccStaticFieldSetByteIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetByte);
    return mccStaticFieldSetByteIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetShortIdx() {
    static GStrIdx mccStaticFieldSetShortIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetShort);
    return mccStaticFieldSetShortIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetCharIdx() {
    static GStrIdx mccStaticFieldSetCharIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetChar);
    return mccStaticFieldSetCharIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetIntIdx() {
    static GStrIdx mccStaticFieldSetIntIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetInt);
    return mccStaticFieldSetIntIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetLongIdx() {
    static GStrIdx mccStaticFieldSetLongIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetLong);
    return mccStaticFieldSetLongIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetFloatIdx() {
    static GStrIdx mccStaticFieldSetFloatIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetFloat);
    return mccStaticFieldSetFloatIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetDoubleIdx() {
    static GStrIdx mccStaticFieldSetDoubleIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetDouble);
    return mccStaticFieldSetDoubleIdx;
  }

  static inline GStrIdx &GetMCCStaticFieldSetObjectIdx() {
    static GStrIdx mccStaticFieldSetObjectIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kMCCStaticFieldSetObject);
    return mccStaticFieldSetObjectIdx;
  }

 private:
  static bool LogicXOR(bool v1, bool v2) {
    return (v1 && !v2) || (!v1 && v2);
  }
};

class FELinkListNode {
 public:
  FELinkListNode();
  virtual ~FELinkListNode();
  void InsertBefore(FELinkListNode *ins);
  void InsertAfter(FELinkListNode *ins);
  static void InsertBefore(FELinkListNode *ins, FELinkListNode *pos);
  static void InsertAfter(FELinkListNode *ins, FELinkListNode *pos);
  static void SpliceNodes(FELinkListNode *head, FELinkListNode *tail, FELinkListNode *newTail);
  FELinkListNode *GetPrev() const {
    return prev;
  }

  void SetPrev(FELinkListNode *node) {
    CHECK_NULL_FATAL(node);
    prev = node;
  }

  void SetPrevNull() {
    prev = nullptr;
  }

  FELinkListNode *GetNext() const {
    return next;
  }

  void SetNextNull() {
    next = nullptr;
  }

  void SetNext(FELinkListNode *node) {
    CHECK_NULL_FATAL(node);
    next = node;
  }

 protected:
  FELinkListNode *prev;
  FELinkListNode *next;
};

class AstSwitchUtil {
 public:
  static AstSwitchUtil &Instance() {
    static AstSwitchUtil local;
    return local;
  }
  std::string CreateEndOrExitLabelName() const;
  void MarkLabelUsed(const std::string &label);
  void MarkLabelUnUsed(const std::string &label);
  void PushNestedBreakLabels(const std::string &label);
  void PopNestedBreakLabels();
  void PushNestedCaseVectors(const std::pair<CaseVector*, LabelIdx> &caseVec);
  void PopNestedCaseVectors();
  bool CheckLabelUsed(const std::string &label);
  const std::pair<CaseVector*, LabelIdx> &GetTopOfNestedCaseVectors() const;
  const std::string &GetTopOfBreakLabels() const;
  std::map<std::string, bool> &GetLabelUseMap() {
    return labelUsed;
  }
  static uint32_t tempVarNo;
  static const char *cleanLabel;
  static const char *exitLabel;
  static const char *blockLabel;
  static const char *caseLabel;
  static const char *catchLabel;
  static const char *endehLabel;

 private:
  AstSwitchUtil() = default;
  ~AstSwitchUtil() = default;
  std::map<std::string, bool> labelUsed = std::map<std::string, bool>();
  std::stack<std::string> nestedBreakLabels = std::stack<std::string>(); // loop and switch blocks
  std::stack<LabelIdx> nestedContinueLabels = std::stack<LabelIdx>(); // loop blocks only
  std::stack<std::pair<CaseVector*, LabelIdx>> nestedCaseVectors = std::stack<std::pair<CaseVector*, LabelIdx>>();
}; // end of AstSwitchUtil

class AstLoopUtil {
 public:
  static AstLoopUtil &Instance() {
    static AstLoopUtil local;
    return local;
  }

  ~AstLoopUtil() = default;
  void PushBreak(std::string labelPair);
  std::string GetCurrentBreak();
  void PopCurrentBreak();
  bool IsBreakLabelsEmpty() const;
  void PushContinue(std::string label);
  std::string GetCurrentContinue();
  bool IsContinueLabelsEmpty() const;
  void PopCurrentContinue();

  bool IsCurrentBreakLabelUsed() {
    return breakLabels.top().second;
  }

  bool IsCurrentContinueLabelUsed() {
    return continueLabels.top().second;
  }

 private:
  AstLoopUtil() = default;
  std::stack<std::pair<std::string, bool>> breakLabels = std::stack<std::pair<std::string, bool>>();
  std::stack<std::pair<std::string, bool>> continueLabels = std::stack<std::pair<std::string, bool>>();
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FE_UTILS_H
