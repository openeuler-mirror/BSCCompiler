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
#include "file_adapter.h"
#include "loader_api.h"
#include "loader/loader_utils.h"
#include "utils/string_utils.h"
#include "base/file_utils.h"
#include "exception/stack_unwinder.h"
#include "interp_support.h"

namespace maplert {
FileAdapter::FileAdapter(const std::string srcPath)
    : originalPath(loaderutils::GetNicePath(srcPath)),
      convertPath(loaderutils::Dex2MplPath(originalPath)),
      isThirdApp(stringutils::WithPrefix(originalPath.c_str(), "/data/app/")),
      isPartialAot(stringutils::WithSuffix(convertPath.c_str(), "/maple/arm64/maplepclasses.so")) {}

FileAdapter::FileAdapter(const std::string srcPath, const IAdapterEx adapter) : FileAdapter(srcPath) {
  pAdapterEx = adapter;
}

ObjFile *FileAdapter::OpenObjectFile(jobject classLoader, bool isFallBack, const std::string designPath) {
  // try to open MFile first
  if (!isFallBack) {
    return pAdapterEx.As<AdapterExAPI*>()->OpenMplFile(classLoader, designPath.empty() ? convertPath : designPath);
  } else {
    if (pAdapterEx.As<AdapterExAPI*>()->IsSystemServer()) {
      return nullptr;  // don't turn-on interpreter for system-server
    }
    // then try to open the DFile
    return pAdapterEx.As<AdapterExAPI*>()->OpenDexFile(classLoader, designPath.empty() ? originalPath : designPath);
  }
}

bool FileAdapter::CloseObjectFile(ObjFile &objFile) {
  return objFile.Close();
}
const ObjFile *FileAdapter::Get(const std::string &path) {
  std::lock_guard<std::mutex> lock(mMplLibLock);
  return GetLocked(path);
}

const ObjFile *FileAdapter::GetLocked(const std::string &path) {
  auto it = mMplLibs.find(path);
  return (it == mMplLibs.end()) ? nullptr : it->second;
}

void FileAdapter::Put(const std::string &path, const ObjFile &objFile) {
  std::lock_guard<std::mutex> lock(mMplLibLock);
  mMplLibs[path] = &objFile;
  if ((objFile.GetFileType() == FileType::kDFile) && (!IsPartialAot())) {
    (void)mMplSeqList.insert(mMplSeqList.begin(), &objFile);
  } else {
    mMplSeqList.push_back(&objFile);
  }
}

bool FileAdapter::Register(const IEnv env, jclass javaClass, const std::string &jniClassName,
    const INativeMethod methods, int32_t methodCount, bool fake) {
  std::lock_guard<std::mutex> lock(mMplLibLock);

  for (const ObjFile *mf : mMplSeqList) {
    ObjFile *objFile = const_cast<ObjFile*>(mf);
    if (mf != nullptr && objFile->CanRegisterNativeMethods(jniClassName) && (objFile->RegisterNativeMethods(env,
        javaClass, jniClassName, methods, methodCount, fake) == true)) {
      return true;
    }
  }
  return false;
}

void FileAdapter::GetObjFiles(std::vector<const ObjFile*> &bootClassPath) {
  std::lock_guard<std::mutex> lock(mMplLibLock);
  (void)std::copy(mMplSeqList.begin(), mMplSeqList.end(), std::back_inserter(bootClassPath));
}

void FileAdapter::GetObjLoaders(std::set<jobject> &classLoaders) {
  std::lock_guard<std::mutex> lock(mMplLibLock);
  std::vector<const ObjFile*>::iterator it = mMplSeqList.begin();
  for (; it != mMplSeqList.end(); ++it) {
    jobject classLoader = (*it)->GetClassLoader();
    (void)classLoaders.insert(classLoader);
  }
}

void FileAdapter::GetObjFileList(std::vector <std::string> &pathList, bool isFallBack) {
  if (!isFallBack) {
    if (isThirdApp) {
      hasStartUp = GetObjFileListInternal(pathList);
    } else {
      if (stringutils::WithSuffix(convertPath.c_str(), ".so")) {
        pathList.push_back(convertPath);
      }
      hasStartUp = false;
    }
  } else {
    hasStartUp = false;
    pAdapterEx.As<AdapterExAPI*>()->GetListOfDexFileToLoad(originalPath, pathList);
  }
  hasSiblings = pathList.size() > 1;
}

bool FileAdapter::GetObjFileListInternal(std::vector <std::string> &pathList) {
  if (!stringutils::WithSuffix(convertPath.c_str(), ".so")) {
    CL_LOG(ERROR) << "base ObjFile name not end with .so: " << convertPath << maple::endl;
    return false;
  }

  // Base ObjFile located in zip, add to path directly, and return
  // Tips: Not support multi-so in zip for performance concern
  if (convertPath.rfind("!/") != std::string::npos) {
    pathList.push_back(convertPath);
    return false;
  }

  size_t posPostfix = convertPath.rfind(".so");

  // Base MplFiles, whose name is mapleclasses.so or maplepclasses.so
  if (maple::FileUtils::FileExists(convertPath)) {
    pathList.push_back(convertPath);
  } else {
    CL_LOG(ERROR) << "cannot find base ObjFile: " << convertPath << maple::endl;
    return false;
  }

  // Sibling MplFiles, whose name starts from 2, like mapleclasses2.so, mapleclass3.so, ... and mapleclass999.so
  for (int i = 2;; ++i) {
    std::string incFileName(convertPath);
    (void)incFileName.insert(posPostfix, std::to_string(i));
    if (maple::FileUtils::FileExists(incFileName)) {
      pathList.push_back(incFileName);
    } else {
      CL_VLOG(classloader) << "cannot find siblings ObjFile: " << incFileName << maple::endl;
      break;
    }
  }

  // Stub ObjFile, whose name index is 999
  std::string stubFileName(convertPath);
  std::string stubIndex("999");
  (void)stubFileName.insert(posPostfix, stubIndex);
  if (maple::FileUtils::FileExists(stubFileName)) {
    pathList.push_back(stubFileName);
  } else {
    CL_VLOG(classloader) << "cannot find stub-siblings ObjFile: " << stubFileName << maple::endl;
  }

  // Starup.so ObjFile.
  std::string fileName = "mapleclasses.so";
  size_t posFileName = convertPath.rfind(fileName);
  if (posFileName == std::string::npos) {
    // Must be "maplepclasses.so"
    return false;
  }
  std::string startupFileName("maplestartup.so");
  std::string startupPathName(convertPath);
  (void)startupPathName.replace(posFileName, fileName.length(), startupFileName);
  if (maple::FileUtils::FileExists(startupPathName)) {
    (void)pathList.insert(pathList.begin(), startupPathName);
    return true;
  } else {
    CL_VLOG(classloader) << "cannot find startup ObjFile: " << startupPathName << maple::endl;
    return false;
  }
}
void FileAdapter::DumpUnregisterNativeFunc(std::ostream &os) {
  std::lock_guard<std::mutex> lock(mMplLibLock);

  for (auto mf : mMplSeqList) {
    ObjFile *objFile = const_cast<ObjFile*>(mf);
    objFile->DumpUnregisterNativeFunc(os);
  }
}
void FileAdapter::DumpMethodName() {
  // We needn't care about the performance of unwinding here.
  std::vector<UnwindContext> uwContextStack;
  // Unwind as many as possible till reaching the end.
  MapleStack::FastRecordCurrentJavaStack(uwContextStack, MAPLE_STACK_UNWIND_STEP_MAX);

  for (auto &uwContext : uwContextStack) {
    std::string methodName;
    if (!uwContext.IsInterpretedContext()) {
      uwContext.frame.GetJavaMethodFullName(methodName);
    } else {
      UnwindContextInterpEx::GetJavaMethodFullNameFromUnwindContext(uwContext, methodName);
    }
    CL_LOG(WARNING) << "LoadClassFromMplFile(), " << methodName << maple::endl;
  }
}
}
