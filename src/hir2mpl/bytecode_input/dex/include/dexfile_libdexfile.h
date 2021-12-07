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
#ifndef HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_LIBDEXFILE_H
#define HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_LIBDEXFILE_H

#include "dexfile_interface.h"
#include <string.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <vector>
#include <list>
#include <base/leb128.h>
#include <dex/dex_file_loader.h>
#include <dex/dex_file.h>
#include <dex/code_item_accessors.h>
#include <dex/dex_file_exception_helpers.h>

namespace maple {
class DexInstruction : public IDexInstruction {
 public:
  explicit DexInstruction(const art::Instruction &artInstruction);
  ~DexInstruction() = default;
  void Init();
  IDexOpcode GetOpcode() const override;
  const char *GetOpcodeName() const override;
  uint32_t GetVRegA() const override;
  uint32_t GetVRegB() const override;
  uint64_t GetVRegBWide() const override;
  uint32_t GetVRegC() const override;
  uint32_t GetVRegH() const override;
  bool HasVRegA() const override;
  bool HasVRegB() const override;
  bool HasWideVRegB() const override;
  bool HasVRegC() const override;
  bool HasVRegH() const override;
  bool HasArgs() const override;
  uint32_t GetArg(uint32_t index) const override;
  IDexInstructionIndexType GetIndexType() const override;
  IDexInstructionFormat GetFormat() const override;
  size_t GetWidth() const override;
  void SetOpcode(IDexOpcode opcode) override;
  void SetVRegB(uint32_t vRegB) override;
  void SetVRegBWide(uint64_t vRegBWide) override;
  void SetIndexType(IDexInstructionIndexType indexType) override;

 private:
  IDexOpcode opcode;
  IDexInstructionIndexType indexType;
  uint32_t vA;
  uint32_t vB;
  uint64_t vBWide;
  uint32_t vC;
  uint32_t vH;
  const art::Instruction *artInstruction = nullptr;  // input parameter
  uint32_t arg[art::Instruction::kMaxVarArgRegs];
};

class LibDexFile : public IDexFile {
 public:
  LibDexFile() = default;
  LibDexFile(std::unique_ptr<const art::DexFile> artDexFile,
             std::unique_ptr<std::string> contentPtrIn);
  ~LibDexFile();
  bool Open(const std::string &fileName) override;
  const void *GetData() const override {
    return dexFile;
  }

  uint32_t GetFileIdx() const override {
    return fileIdx;
  }

  void SetFileIdx(uint32_t fileIdxArg) override {
    fileIdx = fileIdxArg;
  }

  const uint8_t *GetBaseAddress() const override;
  const IDexHeader *GetHeader() const override;
  const IDexMapList *GetMapList() const override;
  uint32_t GetStringDataOffset(uint32_t index) const override;
  uint32_t GetTypeDescriptorIndex(uint32_t index) const override;
  const char *GetStringByIndex(uint32_t index) const override;
  const char *GetStringByTypeIndex(uint32_t index) const override;
  const IDexProtoIdItem *GetProtoIdItem(uint32_t index) const override;
  const IDexFieldIdItem *GetFieldIdItem(uint32_t index) const override;
  const IDexMethodIdItem *GetMethodIdItem(uint32_t index) const override;
  uint32_t GetClassItemsSize() const override;
  const IDexClassItem *GetClassItem(uint32_t index) const override;
  bool IsNoIndex(uint32_t index) const override;
  uint32_t GetTypeIdFromName(const std::string &className) const override;
  uint32_t ReadUnsignedLeb128(const uint8_t **pStream) const override;
  uint32_t FindClassDefIdx(const std::string &descriptor) const override;
  std::unordered_map<std::string, uint32_t> GetDefiningClassNameTypeIdMap() const override;

  // ===== DecodeDebug =====
  void DecodeDebugLocalInfo(const IDexMethodItem &method, DebugNewLocalCallback newLocalCb) override;
  void DecodeDebugPositionInfo(const IDexMethodItem &method, DebugNewPositionCallback newPositionCb) override;

  // ===== Call Site =======
  const ResolvedCallSiteIdItem *GetCallSiteIdItem(uint32_t idx) const override;
  const ResolvedMethodHandleItem *GetMethodHandleItem(uint32_t idx) const override;

 private:
  void DebugNewLocalCb(void *context, const art::DexFile::LocalInfo &entry);
  bool DebugNewPositionCb(void *context, const art::DexFile::PositionInfo &entry);
  bool CheckFileSize(size_t fileSize);
  uint32_t fileIdx = 0;
  // use for save opened content when load on-demand type from opened dexfile
  std::unique_ptr<std::string> contentPtr;
  const art::DexFile *dexFile = nullptr;
  const IDexHeader *header = nullptr;
  const IDexMapList *mapList = nullptr;
  DebugNewLocalCallback debugNewLocalCb = nullptr;
  DebugNewPositionCallback debugNewPositionCb = nullptr;
  std::string content;
  std::vector<std::unique_ptr<const art::DexFile>> dexFiles;
};
}  // namespace maple
#endif  // HIR2MPL_DEX_INPUT_INCLUDE_DEXFILE_LIBDEXFILE_H
