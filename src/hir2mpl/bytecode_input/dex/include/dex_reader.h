/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL_FE_DEX_INPUT_DEX_READER_H
#define MPL_FE_DEX_INPUT_DEX_READER_H
#include <string>
#include <memory>
#include "dexfile_interface.h"
#include "types_def.h"
#include "bc_instruction.h"
#include "bc_class.h"
#include "bc_io.h"

namespace maple {
namespace bc {
typedef std::map<uint16, std::set<std::tuple<std::string, std::string, std::string>>> SrcLocalInfo;

class DexReader : public BCReader {
 public:
  DexReader(uint32 fileIdxIn, const std::string &fileNameIn)
      : BCReader(fileNameIn), fileIdx(fileIdxIn) {}
  ~DexReader() = default;
  void SetDexFile(std::unique_ptr<IDexFile> dexFile);
  uint32 GetClassItemsSize() const;
  const char *GetClassJavaSourceFileName(uint32 classIdx) const;
  bool IsInterface(uint32 classIdx) const;
  uint32 GetClassAccFlag(uint32 classIdx) const;
  std::string GetClassName(uint32 classIdx) const;
  std::list<std::string> GetSuperClasses(uint32 classIdx, bool mapled = false) const;
  std::vector<std::string> GetClassInterfaceNames(uint32 classIdx, bool mapled = false) const;
  std::string GetClassFieldName(uint32 fieldIdx, bool mapled = false) const;
  std::string GetClassFieldTypeName(uint32 fieldIdx, bool mapled = false) const;
  std::string GetClassMethodName(uint32 methodIdx, bool mapled = false) const;
  std::string GetClassMethodDescName(uint32 methodIdx, bool mapled = false) const;
  MapleMap<uint32, BCInstruction*> *ResolveInstructions(MapleAllocator &allocator,
                                                        const IDexMethodItem* dexMethodItem,
                                                        bool mapled = false) const;
  std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> ResolveTryInfos(const IDexMethodItem* dexMethodItem) const;
  void ResovleSrcPositionInfo(const IDexMethodItem* dexMethodItem,
                              std::map<uint32, uint32> &srcPosInfo) const;
  std::unique_ptr<SrcLocalInfo> ResovleSrcLocalInfo(const IDexMethodItem &dexMethodItem) const;
  bool ReadAllDepTypeNames(std::unordered_set<std::string> &depSet);
  bool ReadMethodDepTypeNames(std::unordered_set<std::string> &depSet,
                              uint32 classIdx, uint32 methodItemx, bool isVirtual) const;
  bool ReadAllClassNames(std::unordered_set<std::string> &classSet) const;

  maple::IDexFile &GetIDexFile() const {
    return *iDexFile;
  }
  std::unordered_map<std::string, uint32> GetDefiningClassNameTypeIdMap() const; // for init
  const uint16 *GetMethodInstOffset(const IDexMethodItem* dexMethodItem) const;
  uint16 GetClassMethodRegisterTotalSize(const IDexMethodItem* dexMethodItem) const;
  uint16 GetClassMethodRegisterInSize(const IDexMethodItem* dexMethodItem) const;
  uint32 GetCodeOff(const IDexMethodItem* dexMethodItem) const;

 protected:
  bool OpenAndMapImpl() override;

 private:
  std::string GetStringFromIdxImpl(uint32 idx) const override;
  std::string GetTypeNameFromIdxImpl(uint32 idx) const override;
  ClassElem GetClassMethodFromIdxImpl(uint32 idx) const override;
  ClassElem GetClassFieldFromIdxImpl(uint32 idx) const override;
  std::string GetSignatureImpl(uint32 idx) const override;
  uint32 GetFileIndexImpl() const override;
  MapleMap<uint32, BCInstruction*> *ConstructBCPCInstructionMap(
      MapleAllocator &allocator, const std::map<uint32, std::unique_ptr<IDexInstruction>> &pcInstMap) const;
  BCInstruction *ConstructBCInstruction(MapleAllocator &allocator,
      const std::pair<const uint32, std::unique_ptr<IDexInstruction>> &p) const;
  std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> ConstructBCTryInfoList(
      uint32 codeOff, const std::vector<const IDexTryItem*> &tryItems) const;
  std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> ConstructBCCatchList(
      std::vector<IDexCatchHandlerItem> &catchHandlerItems) const;
  uint32 GetVA(const IDexInstruction *inst) const;
  uint32 GetVB(const IDexInstruction *inst) const;
  uint64 GetWideVB(const IDexInstruction *inst) const;
  uint32 GetVC(const IDexInstruction *inst) const;
  void GetArgVRegs(const IDexInstruction *inst, MapleList<uint32> &vRegs) const;
  uint32 GetVH(const IDexInstruction *inst) const;
  uint8 GetWidth(const IDexInstruction *inst) const;
  const char *GetOpName(const IDexInstruction *inst) const;
  void ReadMethodTryCatchDepTypeNames(std::unordered_set<std::string> &depSet, const IDexMethodItem &method) const;
  void AddDepTypeName(std::unordered_set<std::string> &depSet, const std::string &typeName, bool isTrim) const;

  uint32 fileIdx;
  std::unique_ptr<maple::IDexFile> iDexFile;
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_DEX_INPUT_DEX_READER_H