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
#ifndef MPLFE_INCLUDE_COMMON_FE_OPTIONS_H
#define MPLFE_INCLUDE_COMMON_FE_OPTIONS_H
#include <list>
#include <vector>
#include <string>
#include <set>
#include "mpl_logging.h"
#include "types_def.h"

namespace maple {
class FEOptions {
 public:
  static const int kDumpLevelDisable = 0;
  static const int kDumpLevelInfo = 1;
  static const int kDumpLevelInfoDetail = 2;
  static const int kDumpLevelInfoDebug = 3;

  enum ModeJavaStaticFieldName {
    kNoType = 0,      // without type
    kAllType,         // with type
    kSmart            // auto anti-proguard
  };

  enum ModeCollectDepTypes {
    kAll = 0,      // collect all dependent types
    kFunc          // collect func dependent types
  };

  enum ModeDepSameNamePolicy {
    kSys = 0,      // load type form sys when on-demand load same name type
    kSrc           // load type form src when on-demand load same name type
  };

  enum TypeInferKind {
    kNo = 0,
    kRoahAlgorithm,
    kLinearScan
  };

  static FEOptions &GetInstance() {
    return options;
  }

  void Init() {
    isJBCUseImpreciseType = true;
  }

  // input control options
  void AddInputClassFile(const std::string &fileName);
  const std::list<std::string> &GetInputClassFiles() const {
    return inputClassFiles;
  }

  void AddInputJarFile(const std::string &fileName);
  const std::list<std::string> &GetInputJarFiles() const {
    return inputJarFiles;
  }

  void AddInputDexFile(const std::string &fileName);
  const std::vector<std::string> &GetInputDexFiles() const {
    return inputDexFiles;
  }


  void AddInputMpltFileFromSys(const std::string &fileName) {
    inputMpltFilesFromSys.push_back(fileName);
  }

  const std::list<std::string> &GetInputMpltFilesFromSys() const {
    return inputMpltFilesFromSys;
  }

  void AddInputMpltFileFromApk(const std::string &fileName) {
    inputMpltFilesFromApk.push_back(fileName);
  }

  const std::list<std::string> &GetInputMpltFilesFromApk() const {
    return inputMpltFilesFromApk;
  }

  void AddInputMpltFile(const std::string &fileName) {
    inputMpltFiles.push_back(fileName);
  }

  const std::list<std::string> &GetInputMpltFiles() const {
    return inputMpltFiles;
  }

  // On Demand Type Creation
  void SetXBootClassPath(const std::string &fileName) {
    strXBootClassPath = fileName;
  }

  const std::string &GetXBootClassPath() const {
    return strXBootClassPath;
  }

  void SetClassLoaderContext(const std::string &fileName) {
    strClassLoaderContext = fileName;
  }

  const std::string &GetClassLoaderContext() const {
    return strClassLoaderContext;
  }

  void SetCompileFileName(const std::string &fileName) {
    strCompileFileName = fileName;
  }

  const std::string &GetCompileFileName() const {
    return strCompileFileName;
  }

  void SetModeCollectDepTypes(ModeCollectDepTypes mode) {
    modeCollectDepTypes = mode;
  }

  ModeCollectDepTypes GetModeCollectDepTypes() const {
    return modeCollectDepTypes;
  }

  void SetModeDepSameNamePolicy(ModeDepSameNamePolicy mode) {
    modeDepSameNamePolicy = mode;
  }

  ModeDepSameNamePolicy GetModeDepSameNamePolicy() const {
    return modeDepSameNamePolicy;
  }

  // output control options
  void SetIsGenMpltOnly(bool flag) {
    isGenMpltOnly = flag;
  }

  bool IsGenMpltOnly() const {
    return isGenMpltOnly;
  }

  void SetIsGenAsciiMplt(bool flag) {
    isGenAsciiMplt = flag;
  }

  bool IsGenAsciiMplt() const {
    return isGenAsciiMplt;
  }

  void SetOutputPath(const std::string &path) {
    outputPath = path;
  }

  const std::string &GetOutputPath() const {
    return outputPath;
  }

  void SetOutputName(const std::string &name) {
    outputName = name;
  }

  const std::string &GetOutputName() const {
    return outputName;
  }

  void EnableDumpInstComment() {
    isDumpInstComment = true;
  }

  void DisableDumpInstComment() {
    isDumpInstComment = false;
  }

  bool IsDumpInstComment() const {
    return isDumpInstComment;
  }

  void SetNoMplFile() {
    isNoMplFile = true;
  }

  bool IsNoMplFile() const {
    return isNoMplFile;
  }

  // debug info control options
  void SetDumpLevel(int level) {
    dumpLevel = level;
  }

  int GetDumpLevel() const {
    return dumpLevel;
  }

  void SetIsDumpTime(bool flag) {
    isDumpTime = flag;
  }

  bool IsDumpTime() const {
    return isDumpTime;
  }

  void SetIsDumpComment(bool flag) {
    isDumpComment = flag;
  }

  bool IsDumpComment() const {
    return isDumpComment;
  }

  void SetIsDumpLOC(bool flag) {
    isDumpLOC = flag;
  }

  bool IsDumpLOC() const {
    return isDumpLOC;
  }

  void SetIsDumpPhaseTime(bool flag) {
    isDumpPhaseTime = flag;
  }

  bool IsDumpPhaseTime() const {
    return isDumpPhaseTime;
  }

  void SetIsDumpPhaseTimeDetail(bool flag) {
    isDumpPhaseTimeDetail = flag;
  }

  bool IsDumpPhaseTimeDetail() const {
    return isDumpPhaseTimeDetail;
  }

  // java compiler options
  void SetModeJavaStaticFieldName(ModeJavaStaticFieldName mode) {
    modeJavaStaticField = mode;
  }

  ModeJavaStaticFieldName GetModeJavaStaticFieldName() const {
    return modeJavaStaticField;
  }

  void SetIsJBCUseImpreciseType(bool flag) {
    isJBCUseImpreciseType = flag;
  }

  bool IsJBCUseImpreciseType() const {
    return isJBCUseImpreciseType;
  }

  void SetIsJBCInfoUsePathName(bool flag) {
    isJBCInfoUsePathName = flag;
  }

  bool IsJBCInfoUsePathName() const {
    return isJBCInfoUsePathName;
  }

  void SetIsDumpJBCStmt(bool flag) {
    isDumpJBCStmt = flag;
  }

  bool IsDumpJBCStmt() const {
    return isDumpJBCStmt;
  }

  void SetIsDumpJBCBB(bool flag) {
    isDumpJBCBB = flag;
  }

  bool IsDumpJBCBB() const {
    return isDumpJBCBB;
  }

  void SetIsDumpGeneralCFGGraph(bool flag) {
    isDumpGenCFGGraph = flag;
  }

  bool IsDumpGeneralCFGGraph() const {
    return isDumpGenCFGGraph;
  }

  void SetGeneralCFGGraphFileName(const std::string &fileName) {
    genCFGGraphFileName = fileName;
  }

  const std::string GetJBCCFGGraphFileName() const {
    return genCFGGraphFileName;
  }

  void SetIsDumpJBCAll(bool flag) {
    isDumpJBCAll = flag;
  }

  bool IsDumpJBCAll() const {
    return isDumpJBCAll;
  }

  void SetIsDumpJBCErrorOnly(bool flag) {
    isDumpJBCErrorOnly = flag;
  }

  bool IsDumpJBCErrorOnly() const {
    return isDumpJBCErrorOnly;
  }

  void SetIsEmitJBCLocalVarInfo(bool flag) {
    isEmitJBCLocalVarInfo = flag;
  }

  bool IsEmitJBCLocalVarInfo() const {
    return isEmitJBCLocalVarInfo;
  }

  // parallel
  void SetNThreads(uint32 n) {
    nthreads = n;
  }

  uint32 GetNThreads() const {
    return nthreads;
  }

  void SetDumpThreadTime(bool arg) {
    dumpThreadTime = arg;
  }

  bool IsDumpThreadTime() const {
    return dumpThreadTime;
  }

  void AddDumpJBCFuncName(const std::string &funcName) {
    if (!funcName.empty()) {
      CHECK_FATAL(dumpJBCFuncNames.insert(funcName).second, "dumpJBCFuncNames insert failed");
    }
  }

  const std::set<std::string> &GetDumpJBCFuncNames() const {
    return dumpJBCFuncNames;
  }

  bool IsDumpJBCFuncName(const std::string &funcName) const {
    return dumpJBCFuncNames.find(funcName) != dumpJBCFuncNames.end();
  }

  void SetTypeInferKind(TypeInferKind arg) {
    typeInferKind = arg;
  }

  TypeInferKind GetTypeInferKind() const {
    return typeInferKind;
  }

  bool HasJBC() const {
    return ((inputClassFiles.size() != 0) || (inputJarFiles.size() != 0));
  }

  void SetIsAOT(bool flag) {
    isAOT = flag;
  }

  bool IsAOT() const {
    return isAOT;
  }

 private:
  static FEOptions options;
  // input control options
  std::list<std::string> inputClassFiles;
  std::list<std::string> inputJarFiles;
  std::vector<std::string> inputDexFiles;
  std::list<std::string> inputMpltFilesFromSys;
  std::list<std::string> inputMpltFilesFromApk;
  std::list<std::string> inputMpltFiles;

  // On Demand Type Creation
  std::string strXBootClassPath;
  std::string strClassLoaderContext;
  std::string strCompileFileName;
  ModeCollectDepTypes modeCollectDepTypes = ModeCollectDepTypes::kFunc;
  ModeDepSameNamePolicy modeDepSameNamePolicy = ModeDepSameNamePolicy::kSys;

  // output control options
  bool isGenMpltOnly;
  bool isGenAsciiMplt;
  std::string outputPath;
  std::string outputName;
  bool isDumpInstComment = false;
  bool isNoMplFile = false;

  // debug info control options
  int dumpLevel;
  bool isDumpTime;
  bool isDumpComment = false;
  bool isDumpLOC = false;
  bool isDumpPhaseTime = false;
  bool isDumpPhaseTimeDetail = false;

  // java compiler options
  ModeJavaStaticFieldName modeJavaStaticField = ModeJavaStaticFieldName::kNoType;
  bool isJBCUseImpreciseType = false;
  bool isJBCInfoUsePathName = false;
  bool isDumpJBCStmt = false;
  bool isDumpJBCBB = false;
  bool isDumpJBCAll = false;
  bool isDumpJBCErrorOnly = false;
  std::set<std::string> dumpJBCFuncNames;
  bool isEmitJBCLocalVarInfo = false;

  // general stmt/bb/cfg debug options
  bool isDumpGenCFGGraph = false;
  std::string genCFGGraphFileName = "";

  // parallel
  uint32 nthreads;
  bool dumpThreadTime;

  // type-infer
  TypeInferKind typeInferKind = kLinearScan;

  // symbol resolve
  bool isAOT = false;

  FEOptions();
  ~FEOptions() = default;
};
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_FE_OPTIONS_H
