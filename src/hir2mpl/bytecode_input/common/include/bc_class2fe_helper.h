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

#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_CLASS2FE_HELPER_H
#define HIR2MPL_BC_INPUT_INCLUDE_BC_CLASS2FE_HELPER_H
#include "fe_input_helper.h"
#include "bc_class.h"
#include "bc_pragma.h"
namespace maple {
namespace bc {
class BCClass2FEHelper : public FEInputStructHelper {
 public:
  BCClass2FEHelper(MapleAllocator &allocator, BCClass &klassIn);
  ~BCClass2FEHelper() override = default;

 protected:
  std::string GetStructNameOrinImpl() const override;
  std::string GetStructNameMplImpl() const override;
  std::list<std::string> GetSuperClassNamesImpl() const override;
  std::vector<std::string> GetInterfaceNamesImpl() const override;
  std::string GetSourceFileNameImpl() const override;
  MIRStructType *CreateMIRStructTypeImpl(bool &error) const override;
  uint64 GetRawAccessFlagsImpl() const override;
  GStrIdx GetIRSrcFileSigIdxImpl() const override;
  bool IsMultiDefImpl() const override;
  std::string GetSrcFileNameImpl() const override;

  BCClass &klass;

 private:
  void TryMarkMultiDefClass(MIRStructType &typeImported) const;
};

class BCClassField2FEHelper : public FEInputFieldHelper {
 public:
  BCClassField2FEHelper(MapleAllocator &allocator, const BCClassField &fieldIn)
      : FEInputFieldHelper(allocator),
        field(fieldIn) {}
  ~BCClassField2FEHelper() override = default;
  FieldAttrs AccessFlag2Attribute(uint32 accessFlag) const;

 protected:
  virtual FieldAttrs AccessFlag2AttributeImpl(uint32 accessFlag) const = 0;
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  bool ProcessDeclWithContainerImpl(MapleAllocator &allocator) override;
  const BCClassField &field;
};

class BCClassMethod2FEHelper : public FEInputMethodHelper {
 public:
  BCClassMethod2FEHelper(MapleAllocator &allocator, std::unique_ptr<BCClassMethod> &methodIn);
  ~BCClassMethod2FEHelper() override = default;
  std::unique_ptr<BCClassMethod> &GetMethod() const {
    return method;
  }

 protected:
  bool ProcessDeclImpl(MapleAllocator &allocator) override;
  void SolveReturnAndArgTypesImpl(MapleAllocator &allocator) override;
  std::string GetMethodNameImpl(bool inMpl, bool full) const override;
  bool IsVargImpl() const override;
  bool HasThisImpl() const override;
  MIRType *GetTypeForThisImpl() const override;

  virtual bool IsClinit() const = 0;
  virtual bool IsInit() const = 0;
  bool IsVirtualImpl() const override;
  bool IsNativeImpl() const override;
  bool HasCodeImpl() const override;

  std::unique_ptr<BCClassMethod> &method;
};

class BCInputPragmaHelper : public FEInputPragmaHelper {
 public:
  explicit BCInputPragmaHelper(BCAnnotationsDirectory &bcAnnotationsDirectoryIn)
      : bcAnnotationsDirectory(bcAnnotationsDirectoryIn) {}
  ~BCInputPragmaHelper() override = default;

 protected:
  std::vector<MIRPragma*> &GenerateMIRPragmasImpl() override {
    return bcAnnotationsDirectory.EmitPragmas();
  }

 private:
  BCAnnotationsDirectory &bcAnnotationsDirectory;
};
}  // namespace bc
}  // namespace maple
#endif // HIR2MPL_BC_INPUT_INCLUDE_BC_CLASS2FE_HELPER_H