/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MRT_MAPLERT_INCLUDE_METHODMETA_H_
#define MRT_MAPLERT_INCLUDE_METHODMETA_H_
#include "mobject.h"
#include "base/macros.h"
#include "marray.h"
#include "mstring.h"
#include "argvalue.h"

namespace maplert {
static constexpr uint32_t kMethodHashMask = 0x3FF;
static constexpr uint32_t kMethodFlagBits = 6;
static constexpr uint32_t kMethodFlagVtabIndexBits = 16;

static constexpr uint32_t kDexMethodTag = 0x1;
static constexpr uint32_t kMethodMetaHashMask = 0x3FF;
static constexpr uint32_t kMethodMetaHashBitIdx = 6;
static constexpr uint32_t kMethodMetaAddrIspAddress = 0x02;

// MethodMetaBase： It contain compactMetaFlag to indicate
//                  the struct is compacted or not
// MethodMeta： normal method Meta struct
// MethodMetaCompact: method Meta is compacted by leb128
class MethodAddress {
 public:
  uintptr_t GetAddr() const;
  void SetAddr(uintptr_t address);
  int32_t GetDefTabIndex() const;

 private:
  union {
    DataRefOffset addr;        // func address.
    DataRefOffset defTabIndex; // def tab index, before lazy binding resolve for method.
  };
};

class MethodSignature {
 public:
  char *GetSignature() const;
  void SetSignature(const char *signature);
  MetaRef *GetTypes() const;

 private:
  DataRefOffset32 signatureName;
  // ponit to a types array, only enable Cache ParametarType(methodMeta flag kMethodParametarType is set).
  // otherwise can't use this field.
  DataRefOffset32 pTypes;
};

class MethodMetaBase {
 public:
  char *GetName() const;
  void GetSignature(std::string &signature) const;
  bool IsMethodMetaCompact() const;
  MClass *GetDeclaringClass() const;
  uint16_t GetFlag() const;
  uint32_t GetMod() const;
  int16_t GetVtabIndex() const;
  uintptr_t GetFuncAddress() const;

 private:
  struct {
    int16_t vtabIndex;
    uint16_t compactMetaFlag;
  };
};

class MethodMeta {
 public:
  uint32_t GetMod() const;
  char *GetName() const;
  char *GetSignature() const;
  void GetShortySignature(char shorty[], const uint32_t size) const;
  static void GetShortySignature(const char srcSignature[], char shorty[], const uint32_t size);
  static void GetShortySignatureUtil(const char srcSignature[], char shorty[], const uint32_t size);
  uintptr_t GetFuncAddress() const;
  std::string GetAnnotation() const;
  char *GetAnnotationRaw() const;
  MClass *GetDeclaringClass() const;
  MRT_EXPORT void GetJavaMethodFullName(std::string &name) const;
  MRT_EXPORT void GetJavaClassName(std::string &name) const;
  MRT_EXPORT void GetJavaMethodName(std::string &name) const;
  uint16_t GetArgSize() const;
  int16_t GetVtabIndex() const;
  uint16_t GetHashCode() const;
  void SetHashCode(uint16_t hash);
  uint16_t GetFlag() const;
  bool IsStatic() const;
  bool IsAbstract() const;
  bool IsPublic() const;
  bool IsPrivate() const;
  bool IsDirectMethod() const;
  bool IsFinalizeMethod() const;
  bool IsFinalMethod() const;
  bool IsConstructor() const;
  bool IsDefault() const;
  bool IsProtected() const;
  bool IsNative() const;
  bool IsSynchronized() const;
  bool IsCriticalNative() const;
  bool IsMethodAccessible(const MClass &curClass) const;
  bool IsOverrideMethod(const MethodMeta &srcMethod) const;
  bool IspAddress() const;
  bool IsEnableParametarType() const;
  int32_t GetDefTabIndex() const;
  MethodAddress *GetpMethodAddress() const;
  MClass *GetReturnType() const;
  char *GetReturnTypeName() const;
  char GetReturnPrimitiveType() const;
  void GetParameterTypes(std::vector<MClass*> &parameterTypes) const;
  bool GetParameterTypes(MClass *parameterTypes[], uint32_t size) const;
  bool GetParameterTypesUtil(MClass *parameterTypes[], uint32_t size) const;
  MArray *GetParameterTypes() const;
  MethodSignature *GetMethodSignature() const;
  void GetParameterTypesDescriptor(std::vector<std::string> &descriptors) const;
  void GetExceptionTypes(std::vector<MClass*> &types) const;
  MArray *GetExceptionTypes() const;
  void GetPrettyName(bool needSignature, std::string &dstName) const;
  uint32_t GetParameterCount() const;
  MObject *GetSignatureAnnotation() const;
  bool Cmp(const char *mthName, const char *srcSigName) const;
  bool Cmp(const MString *mthName, const MArray *srcSigName) const;
  bool NameCmp(const MString *mthName) const;
  bool NameCmp(const char *name) const;
  bool SignatureCmp(const MArray *sigArray) const;
  bool SignatureCmp(const char *srcSigName) const;
  void FillMethodMeta(bool copySignature, const char *name, const MethodSignature *mthSignature, const char *annotation,
                      int32_t inVtabIndex, const MClass &methodDclClass, const uintptr_t funcAddr, uint32_t modifier,
                      uint16_t methodFlag, uint16_t argSize);
  template<calljavastubconst::ArgsType argsType>
  static inline void BuildArgstoJvalues(char type, BaseArgValue &values, uintptr_t args);
  template<calljavastubconst::ArgsType argsType>
  inline void BuildJavaMethodArgJvalues(MObject *obj, BaseArgValue &values) const;

  void BuildJValuesArgsFromVaList(jvalue argsJvalue[], va_list valistArgs) const;
  void BuildMArrayArgsFromJValues(MArray &targetValue, jvalue argsJvalue[]) const;
  static void BuildJValuesArgsFromStackMemery(DecodeStackArgs &args, std::string &shorty);
  void BuildJValuesArgsFromStackMemeryPrefixSigNature(DecodeStackArgs &args, std::string prefix);
  // invoke compiled code interface
  template<typename T, calljavastubconst::ArgsType argsType>
  T InvokeJavaMethod(MObject *obj, const uintptr_t methodAargs, uintptr_t calleeFuncAddr = 0) const;
  template<typename T> T InvokeJavaMethodFast(MObject *obj) const; // method without parameter

  template<typename RetType, calljavastubconst::ArgsType argsType, typename T>
  RetType Invoke(MObject *obj, T methodAargs = 0, uintptr_t calleeFuncAddr = 0) const;

  bool NeedsInterp() const;

  // set interfaces
  void SetMod(const uint32_t mod);
  void SetName(const char *name);
  void SetSignature(const char *signature);
  void SetMethodSignature(const MethodSignature *methodSignature);
  void SetAddress(const uintptr_t addr);
  void SetAnnotation(const char *annotation);
  void SetDeclaringClass(const MClass &declaringClass);
  void SetFlag(const uint16_t flag);
  void AddFlag(const uint16_t flag);
  void SetArgsSize(const uint16_t argSize);
  void SetVtabIndex(const int32_t methodVtabIndex);

  MethodSignature *CreateMethodSignatureByName(const char *signature);
#ifndef USE_32BIT_REF
  uint32_t GetPadding() {
    return padding;
  }
#endif
  static constexpr size_t GetModOffset() {
    return offsetof(MethodMeta, mod);
  }
  static constexpr size_t GetDeclaringClassOffset() {
    return offsetof(MethodMeta, declaringClass);
  }
  static constexpr size_t GetAddrOffset() {
    return offsetof(MethodMeta, addr);
  }
  static constexpr size_t GetArgSizeOffset() {
    return offsetof(MethodMeta, argumentSize);
  }

  template<typename T>
  static inline MethodMeta *JniCast(T methodMeta);
  template<typename T>
  static inline MethodMeta *JniCastNonNull(T methodMeta);
  template<typename T>
  static inline MethodMeta *Cast(T methodMeta);
  template<typename T>
  static inline MethodMeta *CastNonNull(T methodMeta);
  inline jmethodID AsJmethodID();

 private:
  struct {
    int16_t vtabIndex;
    uint16_t compactMetaFlag;
  };
  DataRefOffset declaringClass;
  union {
    DataRefOffset pAddr;
    DataRefOffset addr;
  };
  uint32_t mod;
  DataRefOffset32 methodName;
  // only method flag kMethodParametarType is set, we will use pMethodSignature,
  // and compiler will generate MethodSignature struct with pTypes which save types cache,
  // otherwise just use signatureOffset.
  union {
    DataRefOffset32 signatureOffset;
    DataRefOffset32 pMethodSignature;
  };
  DataRefOffset32 annotationValue;
  uint16_t flag;
  uint16_t argumentSize;
#ifndef USE_32BIT_REF
  uint32_t padding;
#endif
};

class MethodMetaCompact {
 public:
  int16_t GetVtabIndex() const;
  uint16_t GetFlag() const;
  uint32_t GetMod() const;
  uintptr_t GetFuncAddress() const;
  const uint8_t *GetpCompact() const;
  bool IsAbstract() const;
  char *GetName() const;
  void GetSignature(std::string &signature) const;
  MClass *GetDeclaringClass() const;
  int32_t GetDefTabIndex() const;
  MethodAddress *GetpMethodAddress() const;
  void SetFuncAddress(uintptr_t address);
  uint8_t *DecodeCompactMethodMeta(const MClass &cls, uintptr_t &funcAddress, uint32_t &modifier,
                                   std::string &methodName, std::string &signatureName,
                                   std::string &annotationValue, int32_t &methodInVtabIndex,
                                   uint16_t &flags, uint16_t &argsSize) const;
  static MethodMeta *DecodeCompactMethodMetas(MClass &cls);
  static uintptr_t GetCompactFuncAddr(const MClass &cls, uint32_t index);
  static MethodMetaCompact *GetMethodMetaCompact(const MClass &cls, uint32_t index);

 private:
  static std::mutex resolveMutex;
  struct {
    int16_t vtabIndex;
    uint16_t compactMetaFlag;
  };
  uint8_t leb128Start;
  // data layout in compact methodmeta
  // ===========================================
  // |  union {int32_t pAddr; int32_t addr;}   |
  // |  int32_t declaringClass                 |
  // |  int32_t modifier                       |
  // |  int32_t methodname                     |
  // |  int32_t arg size                       |
  // |  int32_t methodsignature                |
  // |  int32_t annotation                     |
  // ===========================================
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_METHODMETA_H_
