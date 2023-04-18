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
#ifndef MPL2MPL_INCLUDE_NATIVE_STUB_FUNC_H
#define MPL2MPL_INCLUDE_NATIVE_STUB_FUNC_H
#include "phase_impl.h"
#include "maple_phase_manager.h"

namespace maple {
#ifdef USE_ARM32_MACRO
constexpr int kSlownativeFuncnum = 5;
#else
constexpr int kSlownativeFuncnum = 9;
#endif
// May not use Env or ClassObj
constexpr int kJniTypeNormal = 0;
// Not equal to real critical native, but no need for pre/post/eh.
// Need SetReliableUnwindContext, because the native code may call other java code
constexpr int kJniTypeMapleCriticalNative = 1;
// Equal to real critical native, func will be set critical attribute.
// In theory it's incorrect because passing incorrect args. Ex. Linux.getuid
constexpr int kJniTypeCriticalNative = 2;
// Equal to critical native but need to pass env and classObj. Ex. Character.isDigitImpl
constexpr int kJniTypeCriticalNeedArg = 3;
constexpr int kInvalidCode = 0x01;

class NativeFuncProperty {
 public:
  NativeFuncProperty() = default;
  ~NativeFuncProperty() = default;

 private:
  std::string javaFunc;
  std::string nativeFile;
  std::string nativeFunc;
  int jniType = kJniTypeNormal;
  int useEnv = 1;
  int useClassObj = 1;

  friend class NativeStubFuncGeneration;
};

class NativeStubFuncGeneration : public FuncOptimizeImpl {
 public:
  NativeStubFuncGeneration(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~NativeStubFuncGeneration() override {
    MCCSetReliableUnwindContextFunc = nullptr;
    regTableConst = nullptr;
    MRTDecodeRefFunc = nullptr;
    MRTCallSlowNativeExtFunc = nullptr;
    regFuncTabConst = nullptr;
    MRTPostNativeFunc = nullptr;
    MRTCheckThrowPendingExceptionFunc = nullptr;
    regFuncSymbol = nullptr;
    MRTPreNativeFunc = nullptr;
  }

  void ProcessFunc(MIRFunction *func) override;
  void Finish() override;
  FuncOptimizeImpl *Clone() override {
    return new NativeStubFuncGeneration(*this);
  }

 private:
  bool IsStaticBindingListMode() const;
  bool ReturnsJstr(TyIdx retType) {
    return retType == jstrPointerTypeIdx;
  }
  void InitStaticBindingMethodList();
  bool IsStaticBindingMethod(const std::string &methodName) const;
  void LoadNativeFuncProperty();
  bool GetNativeFuncProperty(const std::string &funcName, NativeFuncProperty &property);
  MIRFunction &GetOrCreateDefaultNativeFunc(MIRFunction &stubFunc);
  void GenerateRegisteredNativeFuncCall(MIRFunction &func, const MIRFunction &nativeFunc, MapleVector<BaseNode*> &args,
                                        const MIRSymbol *ret);
  StmtNode *CreateNativeWrapperCallNode(MIRFunction &func, BaseNode *funcPtr, const MapleVector<BaseNode*> &args,
                                        const MIRSymbol *ret, bool needIndirectCall);
  void GenerateNativeWrapperFuncCall(MIRFunction &func, const MIRFunction &nativeFunc, MapleVector<BaseNode*> &args,
                                     const MIRSymbol *ret);
  void GenerateHelperFuncDecl();
  void GenerateRegTabEntry(const MIRFunction &func);
  void GenerateRegTableEntryType();
  void GenerateRegTable();
  void GenerateRegFuncTabEntryType();
  void GenerateRegFuncTabEntry();
  void GenerateRegFuncTab();
  std::unordered_map<std::string, NativeFuncProperty> nativeFuncPropertyMap;
  // a static binding function list
  std::unordered_set<std::string> staticBindingMethodsSet;
  TyIdx jstrPointerTypeIdx = TyIdx(0);
  MIRAggConst *regTableConst = nullptr;
  MIRSymbol *regFuncSymbol = nullptr;
  MIRAggConst *regFuncTabConst = nullptr;
  MIRFunction *MRTPreNativeFunc = nullptr;
  MIRFunction *MRTPostNativeFunc = nullptr;
  MIRFunction *MRTDecodeRefFunc = nullptr;
  MIRFunction *MRTCheckThrowPendingExceptionFunc = nullptr;
  MIRFunction *MRTCallSlowNativeFunc[kSlownativeFuncnum] = { nullptr };  // for native func which args <=8, use x0-x7
  MIRFunction *MRTCallSlowNativeExtFunc = nullptr;
  MIRFunction *MCCSetReliableUnwindContextFunc = nullptr;
  static const std::string callSlowNativeFuncs[kSlownativeFuncnum];
};

MAPLE_MODULE_PHASE_DECLARE(M2MGenerateNativeStubFunc)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_NATIVE_STUB_FUNC_H
