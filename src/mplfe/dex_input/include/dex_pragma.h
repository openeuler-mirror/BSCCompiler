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
#ifndef MPLFE_BC_INPUT_INCLUDE_DEX_PRAGMA_H
#define MPLFE_BC_INPUT_INCLUDE_DEX_PRAGMA_H
#include <cstdint>
#include <memory>
#include <vector>
#include "bc_pragma.h"
#include "mir_module.h"
#include "mir_const.h"
#include "mempool.h"
#include "dexfile_interface.h"

namespace maple {
namespace bc {
class DexBCAnnotationElement {
 public:
  DexBCAnnotationElement(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg, const uint8_t **annotationDataArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        annotationData(annotationDataArg) {}
  ~DexBCAnnotationElement() {
    annotationData = nullptr;
  }

  static uint64 GetUVal(const uint8 **data, uint8 len) {
    // get value, max 8 bytes, little-endian
    uint64 val = 0;
    for (uint8 j = 0; j <= len; j++) {
      val |= (static_cast<uint64>(*(*data)++) << (j << 3));
    }
    return val;
  }

  MIRPragmaElement *EmitPragmaElement() {
    return ProcessAnnotationElement(annotationData);
  }

 private:
  MIRPragmaElement *ProcessAnnotationElement(const uint8 **data);
  void ProcessAnnotationEncodedValue(const uint8 **data, MIRPragmaElement &element, MIRConst *&cst);
  void ProcessAnnotationEncodedValue(const uint8 **data, MIRPragmaElement &element, PragmaValueType valueType,
                                     uint8 valueArg, MIRConst *&cst);
  MIRIntConst *ProcessAnnotationEncodedValueInternalProcessIntValue(const uint8 **data, MIRPragmaElement &element,
                                                                    uint8 valueArg, MIRType &type);
  MIRStr16Const *ProcessAnnotationEncodedValueInternalProcessStringValue(const uint8 **data, MIRPragmaElement &element,
                                                                         uint8 valueArg);
  void ProcessAnnotationEncodedValueInternalProcessTypeValue(const uint8 **data, MIRPragmaElement &element,
                                                             uint8 valueArg);
  void ProcessAnnotationEncodedValueInternalProcessFieldValue(const uint8 **data, MIRPragmaElement &element,
                                                              uint8 valueArg);
  void ProcessAnnotationEncodedValueInternalProcessMethodValue(const uint8 **data, MIRPragmaElement &element,
                                                               uint8 valueArg);
  MIRAggConst *ProcessAnnotationEncodedValueInternalProcessArrayValue(const uint8 **data, MIRPragmaElement &element);
  void ProcessAnnotationEncodedValueInternalProcessAnnotationValue(const uint8 **data, MIRPragmaElement &element);
  static MIRType *GetTypeFromValueType(PragmaValueType valueType);

  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const uint8_t **annotationData;
};

class DexBCAnnotation {
 public:
  DexBCAnnotation(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg, const IDexAnnotation *iDexAnnotationArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        iDexAnnotation(iDexAnnotationArg) {}
  ~DexBCAnnotation() {
    iDexAnnotation = nullptr;
  }

  MIRPragma *EmitPragma(PragmaKind kind, const GStrIdx &pragIdx, int32 paramNum = -1, const TyIdx &tyIdxEx = TyIdx(0));

 private:
  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const IDexAnnotation *iDexAnnotation;
};

class DexBCAnnotationSet {
 public:
  DexBCAnnotationSet(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg,
                     const IDexAnnotationSet *iDexAnnotationSetArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        iDexAnnotationSet(iDexAnnotationSetArg) {
    Init();
  }

  ~DexBCAnnotationSet() {
    iDexAnnotationSet = nullptr;
    annotations.clear();
  }

  size_t GetSize() const {
    return annotations.size();
  }

  const DexBCAnnotation &GetAnnotation(size_t idx) const {
    return *(annotations[idx]);
  }

  void AddAnnotation(std::unique_ptr<DexBCAnnotation> annotation) {
    annotations.push_back(std::move(annotation));
  }

  bool IsValid() const {
    return iDexAnnotationSet->IsValid();
  }

  std::vector<MIRPragma*> &EmitPragmas(PragmaKind kind, const GStrIdx &pragIdx, int32 paramNum = -1,
                                       const TyIdx &tyIdxEx = TyIdx(0));

 private:
  void Init();
  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const IDexAnnotationSet *iDexAnnotationSet;
  std::vector<std::unique_ptr<DexBCAnnotation>> annotations;
  std::vector<MIRPragma*> pragmas;
};

class DexBCAnnotationSetList {
 public:
  DexBCAnnotationSetList(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg,
                         const IDexAnnotationSetList *iDexAnnotationSetListArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        iDexAnnotationSetList(iDexAnnotationSetListArg) {
    Init();
  }

  ~DexBCAnnotationSetList() {
    iDexAnnotationSetList = nullptr;
    annotationSets.clear();
  }

  std::vector<MIRPragma*> &EmitPragmas(PragmaKind kind, const GStrIdx &pragIdx);

 private:
  void Init();
  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const IDexAnnotationSetList *iDexAnnotationSetList;
  std::vector<std::unique_ptr<DexBCAnnotationSet>> annotationSets;
  std::vector<MIRPragma*> pragmas;
};

class DexBCFieldAnnotations {
 public:
  DexBCFieldAnnotations(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg,
                        const IDexFieldAnnotations *iDexFieldAnnotationsArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        iDexFieldAnnotations(iDexFieldAnnotationsArg) {
    Init();
  }

  ~DexBCFieldAnnotations() {
    iDexFieldAnnotations = nullptr;
  }
  std::vector<MIRPragma*> &EmitPragmas();

 private:
  void Init();
  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const IDexFieldAnnotations *iDexFieldAnnotations;
  std::unique_ptr<DexBCAnnotationSet> annotationSet;
};

class DexBCMethodAnnotations {
 public:
  DexBCMethodAnnotations(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg,
                         const IDexMethodAnnotations *iDexMethodAnnotationsArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        iDexMethodAnnotations(iDexMethodAnnotationsArg) {
    Init();
  }

  ~DexBCMethodAnnotations() {
    iDexMethodAnnotations = nullptr;
  }

  std::vector<MIRPragma*> &EmitPragmas();

 private:
  void Init();
  void SetupFuncAttrs();
  void SetupFuncAttrWithPragma(MIRFunction &mirFunc, const MIRPragma &pragma);
  MIRFunction *GetMIRFunction(const GStrIdx &nameIdx) const;
  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const IDexMethodAnnotations *iDexMethodAnnotations = nullptr;
  std::unique_ptr<DexBCAnnotationSet> annotationSet;
  std::vector<MIRPragma*> *pragmasPtr = nullptr;
  GStrIdx methodFullNameStrIdx;
  const IDexMethodIdItem *methodID = nullptr;
};

class DexBCParameterAnnotations {
 public:
  DexBCParameterAnnotations(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg,
                            const IDexParameterAnnotations *iDexParameterAnnotationsArg)
      : module(moduleArg),
        mp(mpArg),
        iDexFile(iDexFileArg),
        iDexParameterAnnotations(iDexParameterAnnotationsArg) {
    Init();
  }

  virtual ~DexBCParameterAnnotations() = default;
  std::vector<MIRPragma*> &EmitPragmas();

 private:
  void Init();
  MIRModule &module;
  MemPool &mp;
  IDexFile &iDexFile;
  const IDexParameterAnnotations *iDexParameterAnnotations;
  std::unique_ptr<DexBCAnnotationSetList> annotationSetList;
  std::vector<MIRPragma*> pragmas;
};

class DexBCAnnotationsDirectory : public BCAnnotationsDirectory {
 public:
  DexBCAnnotationsDirectory(MIRModule &moduleArg, MemPool &mpArg, IDexFile &iDexFileArg,
                            const std::string &classNameArg,
                            const IDexAnnotationsDirectory *iDexAnnotationsDirectoryArg)
      : BCAnnotationsDirectory(moduleArg, mpArg),
        iDexFile(iDexFileArg),
        className(classNameArg),
        iDexAnnotationsDirectory(iDexAnnotationsDirectoryArg) {
    Init();
  }

  ~DexBCAnnotationsDirectory() {
    iDexAnnotationsDirectory = nullptr;
  }

 protected:
  std::vector<MIRPragma*> &EmitPragmasImpl() override;

 private:
  void Init();
  IDexFile &iDexFile;
  std::string className;
  const IDexAnnotationsDirectory *iDexAnnotationsDirectory;
  std::unique_ptr<DexBCAnnotationSet> classAnnotationSet;
  std::vector<std::unique_ptr<DexBCFieldAnnotations>> fieldAnnotationsItems;
  std::vector<std::unique_ptr<DexBCMethodAnnotations>> methodAnnotationsItems;
  std::vector<std::unique_ptr<DexBCParameterAnnotations>> parameterAnnotationsItems;
};
}  // namespace bc
}  // namespace maple
#endif // MPLFE_BC_INPUT_INCLUDE_DEX_PRAGMA_H