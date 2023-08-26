/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_REFLECTION_ANALYSIS_H
#define MPL2MPL_INCLUDE_REFLECTION_ANALYSIS_H
#include "class_hierarchy_phase.h"

namespace maple {
// maple field index definition
enum class ClassRO : uint32 {
  kClassName,
  kIfields,
  kMethods,
  kSuperclass,
  kNumOfFields,
  kNumofMethods,
#ifndef USE_32BIT_REF
  kFlag,
  kNumOfSup,
  kPadding,
#endif  //! USE_32BIT_REF
  kMod,
  kAnnotation,
  kClinitAddr
};

enum StaticFieldName {
  kClassName = 0,
  kFieldName = 1,
  kTypeName = 2,
};

enum class ClassProperty : uint32 {
  kShadow,
  kMonitor,
  kClassloader,
  kObjsize,
#ifdef USE_32BIT_REF
  FLAG,
  NUMOFSUP,
#endif  // USE_32BIT_REF
  kItab,
  kVtab,
  kGctib,
  kInfoRo,
#ifdef USE_32BIT_REF
  kInstanceOfCacheFalse,
#endif
  kClint
};

enum class MethodProperty : uint32 {
  kVtabIndex,
  kDeclarclass,
  kPaddrData,
  kMod,
  kMethodName,
  kSigName,
  kAnnotation,
  kFlag,
  kArgsize,
#ifndef USE_32BIT_REF
  kPadding
#endif
};

enum class MethodInfoCompact : uint32 {
  kVtabIndex,
  kPaddrData
};

enum class FieldProperty : uint32 {
  kPOffset,
  kMod,
  kFlag,
  kIndex,
  kTypeName,
  kName,
  kAnnotation,
  kDeclarclass,
  kPClassType
};

enum class FieldPropertyCompact : uint32 {
  kPOffset,
  kMod,
  kTypeName,
  kIndex,
  kName,
  kAnnotation
};

enum class MethodSignatureProperty : uint32 {
  kSignatureOffset,
  kParameterTypes
};

class ReflectionAnalysis : public AnalysisResult {
 public:
  ReflectionAnalysis(MIRModule *mod, MemPool *memPool, KlassHierarchy *kh, MIRBuilder &builder)
      : AnalysisResult(memPool),
        mirModule(mod),
        allocator(memPool),
        klassH(kh),
        mirBuilder(builder),
        classTab(allocator.Adapter()),
        mapMethodSignature(allocator.Adapter()),
        isLibcore(false),
        isLazyBindingOrDecouple(false),
        reflectionMuidStr(allocator.GetMemPool()) {
    Klass *objectKlass = kh->GetKlassFromLiteral(namemangler::kJavaLangObjectStr);
    if (objectKlass != nullptr && objectKlass->GetMIRStructType()->IsLocal()) {
      isLibcore = true;
    }
  }
  ~ReflectionAnalysis() override {
    klassH = nullptr;
    mirModule = nullptr;
  }

  static void GenStrTab(MIRModule &module);
  static uint32 FindOrInsertRepeatString(const std::string &str, bool isHot = false, uint8 hotType = kLayoutUnused);
  static BaseNode *GenClassInfoAddr(BaseNode *obj, MIRBuilder &builder);
  void Run();
  static void ConvertMethodSig(std::string &signature);
  static TyIdx GetClassMetaDataTyIdx() {
    return classMetadataTyIdx;
  }
  void DumpPGOSummary() const;

 private:
  static std::unordered_map<std::string, uint32> &GetStr2IdxMap() {
    return str2IdxMap;
  }

  static void SetStr2IdxMap(const std::string &str, uint32 index) {
    str2IdxMap[str] = index;
  }

  static const std::string &GetStrTab() {
    return strTab;
  }

  static const std::string &GetStrTabStartHot() {
    return strTabStartHot;
  }

  static const std::string &GetStrTabBothHot() {
    return strTabBothHot;
  }

  static const std::string &GetStrTabRunHot() {
    return strTabRunHot;
  }

  static void AddStrTab(const std::string &str) {
    strTab += str;
  }

  static void AddStrTabStartHot(const std::string &str) {
    strTabStartHot += str;
  }

  static void AddStrTabBothHot(const std::string &str) {
    strTabBothHot += str;
  }

  static void AddStrTabRunHot(const std::string &str) {
    strTabRunHot += str;
  }

  static uint32 FirstFindOrInsertRepeatString(const std::string &str, bool isHot, uint8 hotType);
  MIRSymbol *GetOrCreateSymbol(const std::string &name, const TyIdx &tyIdx, bool needInit) const;
  MIRSymbol *GetSymbol(const std::string &name, const TyIdx &tyIdx) const;
  MIRSymbol *CreateSymbol(GStrIdx strIdx, TyIdx tyIdx) const;
  MIRSymbol *GetSymbol(GStrIdx strIdx, TyIdx tyIdx) const;
  void GenClassMetaData(Klass &klass);
  std::string GetAnnoValueNoArray(const MIRPragmaElement &annoElem);
  std::string GetArrayValue(const MapleVector<MIRPragmaElement*> &subelemVector);
  std::string GetAnnotationValue(const MapleVector<MIRPragmaElement*> &subelemVector, GStrIdx typeStrIdx);
  MIRSymbol *GenSuperClassMetaData(std::list<Klass*> superClassList);
  MIRSymbol *GenFieldOffsetData(const Klass &klass, const std::pair<FieldPair, int> &fieldInfo);
  MIRSymbol *GetMethodSignatureSymbol(std::string signature);
  MIRSymbol *GetParameterTypesSymbol(uint32 size, uint32 index);
  MIRSymbol *GenFieldsMetaData(const Klass &klass, bool isHot);
  MIRSymbol *GenMethodsMetaData(const Klass &klass, bool isHot);
  MIRSymbol *GenFieldsMetaCompact(const Klass &klass, std::vector<std::pair<FieldPair, int>> &fieldsVector);
  void GenFieldMetaCompact(const Klass &klass, MIRStructType &fieldsInfoCompactType,
                           const std::pair<FieldPair, int> &fieldInfo,
                           MIRAggConst &aggConstCompact);
  void GenMethodMetaCompact(const Klass &klass, MIRStructType &methodsInfoCompactType, int idx,
                            const MIRSymbol &funcSym, MIRAggConst &aggConst,
                            int &allDeclaringClassOffset,
                            std::unordered_map<uint32, std::string> &baseNameMp,
                            std::unordered_map<uint32, std::string> &fullNameMp);
  MIRSymbol *GenMethodsMetaCompact(const Klass &klass,
                                   std::vector<std::pair<MethodPair*, int>> &methodInfoVec,
                                   std::unordered_map<uint32, std::string> &baseNameMp,
                                   std::unordered_map<uint32, std::string> &fullNameMp);
  MIRSymbol *GenFieldsMeta(const Klass &klass, std::vector<std::pair<FieldPair, int>> &fieldsVector,
                           const std::vector<std::pair<FieldPair, uint16>> &fieldHashVec);
  void GenFieldMeta(const Klass &klass, MIRStructType &fieldsInfoType,
                    const std::pair<FieldPair, int> &fieldInfo, MIRAggConst &aggConst,
                    int idx, const std::vector<std::pair<FieldPair, uint16>> &fieldHashVec);
  MIRSymbol *GenMethodsMeta(const Klass &klass, std::vector<std::pair<MethodPair*, int>> &methodInfoVec,
                            std::unordered_map<uint32, std::string> &baseNameMp,
                            std::unordered_map<uint32, std::string> &fullNameMp);
  void GenMethodMeta(const Klass &klass, MIRStructType &methodsInfoType,
                     const MIRSymbol &funcSym, MIRAggConst &aggConst, int idx,
                     std::unordered_map<uint32, std::string> &baseNameMp,
                     std::unordered_map<uint32, std::string> &fullNameMp);
  uint32 GetMethodModifier(const Klass &klass, const MIRFunction &func) const;
  uint32 GetMethodFlag(const MIRFunction &func) const;
  void GenFieldOffsetConst(MIRAggConst &newConst, const Klass &klass, const MIRStructType &type,
                           const std::pair<FieldPair, int> &fieldInfo, uint32 metaFieldID);
  MIRSymbol *GenMethodAddrData(const MIRSymbol &funcSym);
  static void GenMetadataType(MIRModule &module);
  static MIRType *GetRefFieldType();
  static TyIdx GenMetaStructType(MIRModule &module, MIRStructType &metaType, const std::string &str);
  uint32 GetHashIndex(const std::string &strName) const;
  static void GenHotClassNameString(const Klass &klass);
  uint32 FindOrInsertReflectString(const std::string &str) const;
  static void InitReflectString();
  uint32 BKDRHash(const std::string &strName, uint32 seed) const;
  void GenClassHashMetaData();
  void MarkWeakMethods() const;

  bool VtableFunc(const MIRFunction &func) const;
  void GenPrimitiveClass();
  void GenAllMethodHash(std::vector<std::pair<MethodPair*, int>> &methodInfoVec,
                        std::unordered_map<uint32, std::string> &baseNameMap,
                        std::unordered_map<uint32, std::string> &fullNameMap) const;
  void GenAllFieldHash(std::vector<std::pair<FieldPair, uint16>> &fieldV) const;
  void GenAnnotation(std::map<int, int> &idxNumMap, std::string &annoArr, MIRStructType &classType,
                      PragmaKind paragKind, const std::string &paragName, TyIdx fieldTypeIdx,
                      std::map<int, int> *paramnumArray = nullptr, int *paramIndex = nullptr);
  void AppendValueByType(std::string &annoArr, const MIRPragmaElement &elem) const;
  bool IsAnonymousClass(const std::string &annotationString) const;
  bool IsLocalClass(const std::string annotationString) const;
  bool IsPrivateClass(const MIRClassType &classType) const;
  bool IsStaticClass(const MIRStructType &classType) const;
  int8 JudgePara(MIRStructType &classType) const;
  void CheckPrivateInnerAndNoSubClass(Klass &clazz, const std::string &annoArr) const;
  void ConvertMapleClassName(const std::string &mplClassName, std::string &javaDsp) const;

  int GetDeflateStringIdx(const std::string &subStr, bool needSpecialFlag) const;
  uint32 GetAnnoCstrIndex(std::map<int, int> &idxNumMap, const std::string &annoArr, bool isField) const;
  uint16 GetMethodInVtabIndex(const Klass &klass, const MIRFunction &func) const;
  void GetSignatureTypeNames(std::string &signature, std::vector<std::string> &typeNames);
  MIRSymbol *GetClinitFuncSymbol(const Klass &klass) const;
  int SolveAnnotation(MIRStructType &classType, const MIRFunction &func);
  uint32 GetTypeNameIdxFromType(const MIRType &type, const Klass &klass, const std::string &fieldName) const;
  bool IsMemberClass(const std::string &annotationString) const;
  int8_t GetAnnoFlag(const std::string &annotationString) const;
  void GenFieldTypeClassInfo(const MIRType &type, const Klass &klass, std::string &classInfo,
                             const std::string fieldName, bool &isClass) const;

  MIRModule *mirModule;
  MapleAllocator allocator;
  KlassHierarchy *klassH;
  MIRBuilder &mirBuilder;
  MapleVector<MIRSymbol*> classTab;
  MapleMap<MapleString, MIRSymbol*> mapMethodSignature;
  bool isLibcore;
  bool isLazyBindingOrDecouple;
  MapleString reflectionMuidStr;
  static const char *klassPtrName;
  static TyIdx classMetadataTyIdx;
  static TyIdx classMetadataRoTyIdx;
  static TyIdx methodsInfoTyIdx;
  static TyIdx methodsInfoCompactTyIdx;
  static TyIdx fieldsInfoTyIdx;
  static TyIdx fieldsInfoCompactTyIdx;
  static TyIdx superclassMetadataTyIdx;
  static TyIdx fieldOffsetDataTyIdx;
  static TyIdx methodAddrDataTyIdx;
  static TyIdx methodSignatureTyIdx;
  static std::string strTab;
  static std::unordered_map<std::string, uint32> str2IdxMap;
  static std::string strTabStartHot;
  static std::string strTabBothHot;
  static std::string strTabRunHot;
  static bool strTabInited;
  static TyIdx invalidIdx;
  static constexpr uint16 kNoHashBits = 6u;
  // profile statistics
  static uint32_t hotMethodMeta;
  static uint32_t totalMethodMeta;
  static uint32_t hotClassMeta;
  static uint32_t totalClassMeta;
  static uint32_t hotFieldMeta;
  static uint32_t totalFieldMeta;
  static uint32_t hotCStr;
  static uint32_t totalCStr;
  static constexpr char annoDelimiterPrefix = '`';
  static constexpr char annoDelimiter = '!';
  static constexpr char annoArrayStartDelimiter = '{';
  static constexpr char annoArrayEndDelimiter = '}';
  static std::map<std::list<Klass*>, std::string> superClasesIdxMap;
};

MAPLE_MODULE_PHASE_DECLARE(M2MReflectionAnalysis)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_REFLECTION_ANALYSIS_H
