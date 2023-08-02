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
#include "mrt_profile.h"
#include <cstring>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <unordered_set>
#include <set>
#include <vector>
#include <sstream>
#include "mclass_inline.h"
#include "fieldmeta_inline.h"
#include "methodmeta_inline.h"
#include "profile_type.h"
#include "file_layout.h"
#include "linker_api.h"
#include "utils/time_utils.h"
#include "mrt_string.h"
#include "gc_log.h"
using namespace maple;

namespace maplert {
namespace {
std::unordered_set<const MClass*> hotClassMetaSet;
std::unordered_set<const MClass*> hotMethodMetaSet;
std::unordered_set<const MClass*> hotFieldMetaSet;
std::unordered_set<const MethodMeta*> hotMethodSignatureSet;
std::unordered_map<const char*, LayoutType> hotRefectionStrInfo;
std::vector<std::string> strTab;
std::unordered_map<std::string, uint32_t> str2idx;
const char *kMapleSoPrefix = "libmaple";
std::atomic<bool> runPhase(false);
std::mutex reflectStrMtx;
std::mutex classMetaMtx;
std::mutex fieldMetaMtx;
std::mutex methodMetaMtx;
std::mutex methodSignatureMtx;
std::atomic<bool> profileEnable(true);
}

template<typename T> using MapleProfile = std::unordered_map<uint32_t, std::vector<T>>;
using MapleIRProf = std::unordered_map<std::string, MFileIRProfInfo>;

static uint32_t GetorInsertStrInTab(const std::string &str) {
  static uint32_t idx = 0;
  auto item = str2idx.find(str);
  if (item == str2idx.end()) {
    uint32_t oldIdx = idx;
    str2idx.insert(std::make_pair(str, oldIdx));
    strTab.push_back(str);
    idx++;
    return oldIdx;
  } else {
    return item->second;
  }
}

template<typename Out>
static void Split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}
// system/lib64/libmaplecore-all.so  --> core-all
static std::string GetBaseName(const std::string &str) {
  size_t pos = str.find(kMapleSoPrefix);
  std::string baseName(str);
  if (pos != std::string::npos) {
    size_t posEnd = str.find_last_of('.');
    baseName = str.substr(pos + strlen(kMapleSoPrefix), posEnd - pos - strlen(kMapleSoPrefix));
  }
  if (!LOG_NDEBUG) {
    LOG(INFO) << "Base name of " << str << " is " << baseName << maple::endl;
  }
  return baseName;
}
extern "C" void MRT_EnableMetaProfile() {
  profileEnable = true;
}

extern "C" void MRT_DisableMetaProfile() {
  profileEnable = false;
}

extern "C" void MRT_ClearMetaProfile() {
  runPhase  = true;
}

void InsertReflectionString(const char *kHotString) {
  if (kHotString == nullptr) {
    return;
  }
  if (profileEnable) {
    std::lock_guard<std::mutex> lock(reflectStrMtx);
    if (runPhase) {
      auto item = hotRefectionStrInfo.find(kHotString);
      if (item != hotRefectionStrInfo.end()) {
        if (item->second == kLayoutBootHot) {
          item->second = kLayoutBothHot;
        } else {
          return ;
        }
      } else {
        hotRefectionStrInfo.insert(std::make_pair(kHotString, kLayoutRunHot));
      }
    } else {
      hotRefectionStrInfo.insert(std::make_pair(kHotString, kLayoutBootHot));
    }
  }
}

void InsertClassMetadata(const MClass &klass) {
  std::lock_guard<std::mutex> lock(classMetaMtx);
  (void)hotClassMetaSet.insert(&klass);
}

void InsertMethodMetadata(const MethodMetaBase *kMethod) {
  if (kMethod == nullptr) {
    return;
  }
  if (profileEnable) {
    MClass *declaringClass = kMethod->GetDeclaringClass();
    std::lock_guard<std::mutex> lock(methodMetaMtx);
    hotMethodMetaSet.insert(declaringClass);
  }
}

void InsertMethodMetadata(const MClass &cls) {
  std::lock_guard<std::mutex> lock(methodMetaMtx);
  (void)hotMethodMetaSet.insert(&cls);
}

void InsertFieldMetadata(FieldMeta *fieldMeta) {
  if (fieldMeta == nullptr) {
    return;
  }
  MClass *declearingclass = fieldMeta->GetDeclaringclass();
  std::lock_guard<std::mutex> lock(fieldMetaMtx);
  hotFieldMetaSet.insert(declearingclass);
}

void InsertFieldMetadata(const MClass &cls) {
  std::lock_guard<std::mutex> lock(fieldMetaMtx);
  (void)hotFieldMetaSet.insert(&cls);
}

void InsertMethodSignature(const MethodMeta &method) {
  std::lock_guard<std::mutex> lock(methodSignatureMtx);
  (void)hotMethodSignatureSet.insert(&method);
}

void GenStrData(std::vector<char> &strData) {
  for (auto str : strTab) {
    strData.insert(strData.end(), str.c_str(), str.c_str() + str.size() + 1);
  }
}

class ProfileWriter {
 public:
  ProfileWriter(const std::string &path, bool isSystemServer) : path(path), isSystemServer(isSystemServer) {}
  ~ProfileWriter() = default;
  void WriteToFile();
 private:
  std::string path;
  bool isSystemServer = false;
  uint8_t profileDataNum = 0;
  uint32_t profileDataSize = 0;
  static constexpr uint64_t nsPerUs = 1000UL;
  std::vector<char> profileData;
  std::vector<ProfileDataInfo> profileHeaders;
  void RemoveProfile();
  void GenReflectStrMetaData(MapleProfile<ReflectionStrItem> &metaProfile,
                             std::unordered_map<const char*, LayoutType> &data);
  void GenMetaData(MapleProfile<MetaItem> &metaProfile, std::unordered_set<const MClass*> &data);
  void GenFuncData(MapleProfile<FunctionItem> &funcProfile,
                   std::unordered_map<std::string, std::vector<FuncProfInfo>> &data);
  void GenMethodSignatureData(MapleProfile<MethodSignatureItem> &metaProfile,
                              std::unordered_set<const MethodMeta*> &data);
  template<class T>
  void WriteProfileData(MapleProfile<T> &data, ProfileType profileType);
  void WriteIRProfData(MapleIRProf &data);
  void ProcessMetaData();
  void ProcessFuncProf();
  void ProcessRawProf();
  void InitFileHeader(Header &header);
  void Init();
  void ProcessProfData(); // used to transfer the raw profdata to file format
  void WriteFormatDataToFile();
};

void ProfileWriter::GenReflectStrMetaData(MapleProfile<ReflectionStrItem> &metaProfile,
                                          std::unordered_map<const char*, LayoutType> &data) {
  for (auto item : data) {
    void *pc = static_cast<void*>(const_cast<char*>(item.first));
    LinkerMFileInfo *soInfo = LinkerAPI::Instance().GetLinkerMFileInfoByAddress(pc, false);
    if (soInfo == nullptr) {
      LOG(ERROR) << "find reflect str so failed  " << std::hex << pc << std::dec << maple::endl;
      continue;
    }
    // skip system so when save app profile
    if (!isSystemServer && !soInfo->BelongsToApp()) {
      continue;
    }
    std::string belongSo = soInfo->name;
    std::string baseName = GetBaseName(belongSo);
    std::string name = item.first;
    uint32_t soIdx = GetorInsertStrInTab(baseName);
    uint32_t strIdx = GetorInsertStrInTab(name);
    auto metaData = metaProfile.find(soIdx);
    if (metaData == metaProfile.end()) {
      std::vector<ReflectionStrItem> metaList { ReflectionStrItem(strIdx, static_cast<uint8_t>(item.second)) };
      metaProfile.insert(std::make_pair(soIdx, metaList));
    } else {
      (metaData->second).emplace_back(strIdx, item.second);
    }
  }
}

// gen meta data from the raw data
void ProfileWriter::GenMetaData(MapleProfile<MetaItem> &metaProfile, std::unordered_set<const MClass*> &data) {
  for (auto item : data) {
    LinkerMFileInfo *soInfo = LinkerAPI::Instance().GetLinkerMFileInfoByAddress(item, false);
    if (soInfo == nullptr) {
      LOG(ERROR) << "find Meta str so failed  " << std::hex << item << std::dec << maple::endl;
      continue;
    }
    if (!isSystemServer && !soInfo->BelongsToApp()) {
      continue;
    }
    std::string belongSo = soInfo->name;
    std::string baseName = GetBaseName(belongSo);
    char *className = item->GetName();
    std::string name = className;
    uint32_t soIdx = GetorInsertStrInTab(baseName);
    uint32_t strIdx = GetorInsertStrInTab(name);
    auto metaData = metaProfile.find(soIdx);
    if (metaData == metaProfile.end()) {
      std::vector<MetaItem> metaList { MetaItem(strIdx) };
      metaProfile.insert(std::make_pair(soIdx, metaList));
    } else {
      (metaData->second).emplace_back(strIdx);
    }
  }
}

void ProfileWriter::GenFuncData(MapleProfile<FunctionItem> &funcProfile,
                                std::unordered_map<std::string, std::vector<FuncProfInfo>> &data) {
  for (auto item : data) {
    auto &rawFunclist = item.second;
    std::string baseName = GetBaseName(item.first);
    uint32_t soIdx = GetorInsertStrInTab(baseName);
    auto &list = funcProfile[soIdx];
    for (auto &funcProfileData : rawFunclist) {
      std::vector<std::string> soNames;
      Split(funcProfileData.funcName, '|', std::back_inserter(soNames));
      // funcname ==> className|funcName|signaturename
      // so className idx in vector is 0, funcName is 1, signatureName is 2
      uint32_t classIdx = GetorInsertStrInTab(soNames[0]);
      uint32_t funcIdx = GetorInsertStrInTab(soNames[1]);
      uint32_t sigIdx = GetorInsertStrInTab(soNames[2]);
      list.emplace_back(classIdx, funcIdx, sigIdx, funcProfileData.callTimes, funcProfileData.layoutType);
    }
  }
}

void ProfileWriter::GenMethodSignatureData(MapleProfile<MethodSignatureItem> &metaProfile,
                                           std::unordered_set<const MethodMeta*> &data) {
  for (auto item : data) {
    LinkerMFileInfo *metghodSoInfo = LinkerAPI::Instance().GetLinkerMFileInfoByAddress(item, false);
    LinkerMFileInfo *sigSoInfo = LinkerAPI::Instance().GetLinkerMFileInfoByAddress(item->GetSignature(), false);
    LinkerMFileInfo *soInfo;
    if (sigSoInfo == nullptr && metghodSoInfo == nullptr) {
      LOG(ERROR) << "find Meta str so failed  " << std::hex << item << std::dec << maple::endl;
      continue;
    } else if (metghodSoInfo == nullptr) {
      soInfo = sigSoInfo;
    } else {
      soInfo = metghodSoInfo;
    }
    if (!isSystemServer && !soInfo->BelongsToApp()) {
      continue;
    }
    std::string belongSo = soInfo->name;
    std::string baseName = GetBaseName(belongSo);
    char *methodName = item->GetName();
    char *signatureName = item->GetSignature();
    uint32_t soIdx = GetorInsertStrInTab(baseName);
    uint32_t methodIdx = GetorInsertStrInTab(methodName);
    uint32_t sigIdx = GetorInsertStrInTab(signatureName);
    auto metaData = metaProfile.find(soIdx);
    if (metaData == metaProfile.end()) {
      std::vector<MethodSignatureItem> metaList { MethodSignatureItem(methodIdx, sigIdx) };
      metaProfile.insert(std::make_pair(soIdx, metaList));
    } else {
      (metaData->second).emplace_back(methodIdx, sigIdx);
    }
  }
}

// profiledata are arranged by [header][dataArray][header][dataArray] and so on
template<typename T>
void ProfileWriter::WriteProfileData(MapleProfile<T> &data, ProfileType profileType) {
  uint32_t realNum = 0;
  uint32_t lastProfileDataSize = profileDataSize;
  for (auto &item : data) {
    auto typeTProfdataArray = item.second;
    uint32_t num = static_cast<uint32_t>(typeTProfdataArray.size());
    // if size overflow, or num is 0 skip this data
    if ((num > (UINT32_MAX / sizeof(T))) || num == 0) {
      continue;
    }
    realNum++;
    char *str = nullptr;
    uint32_t soIdx = item.first;
    str = reinterpret_cast<char*>(&soIdx);
    profileData.insert(profileData.end(), str, str + sizeof(soIdx));
    uint32_t size = num * sizeof(T);

    str = reinterpret_cast<char*>(&num);
    profileData.insert(profileData.end(), str, str + sizeof(num));

    str = reinterpret_cast<char*>(&size);
    profileData.insert(profileData.end(), str, str + sizeof(size));

    str = reinterpret_cast<char*>(typeTProfdataArray.data());
    profileData.insert(profileData.end(), str, str + size);
    // update profileDataSize
    profileDataSize  = profileDataSize + sizeof(num) + sizeof(size) + sizeof(soIdx) + size;;
  }
  if (realNum != 0) {
    profileHeaders.emplace_back(lastProfileDataSize, static_cast<uint8_t>(profileType), realNum);
    profileDataNum++;
  } else {
    return ;
  }
}

void ProfileWriter::WriteIRProfData(MapleIRProf &data) {
  MapleProfile<FunctionIRProfItem> funcDescTab;
  MapleProfile<FuncCounterItem> funcCounterTab;
  for (auto item : data) {
    auto &rawFuncList = item.second;
    std::string baseName = GetBaseName(item.first);
    uint32_t soIdx = GetorInsertStrInTab(baseName);
    auto &descTab = rawFuncList.descTab;
    auto &counterTab = rawFuncList.counterTab;
    // gen file counter tab
    auto &counterList = funcCounterTab[soIdx];
    for (auto &counter : counterTab) {
      counterList.emplace_back(counter);
    }

    // gen file func desc tab
    auto &list = funcDescTab[soIdx];
    for (auto &funcDesc : descTab) {
      std::vector<std::string> soNames;
      Split(funcDesc.funcName, '|', std::back_inserter(soNames));
      uint32_t classIdx = GetorInsertStrInTab(soNames[0]);
      uint32_t funcIdx = GetorInsertStrInTab(soNames[1]);
      uint32_t sigIdx = GetorInsertStrInTab(soNames[2]);
      list.emplace_back(funcDesc.hash, classIdx, funcIdx, sigIdx, funcDesc.start, funcDesc.end);
    }
    VLOG(profiler) << baseName << " counterTab size " << counterList.size()  << " descTab size " <<
        list.size() <<  "\n";
  }
  // because profile desc tab depends on profile counter Tab, so must write counterTab first
  WriteProfileData<FuncCounterItem>(funcCounterTab, kIRCounter);
  WriteProfileData<FunctionIRProfItem>(funcDescTab, kBBInfo);
}

// process classmeta/fieldmeta/methodmeta/reflectionstr profile data;
void ProfileWriter::ProcessMetaData() {
  MapleProfile<MetaItem> classMeta;
  {
    std::lock_guard<std::mutex> lock(classMetaMtx);
    GenMetaData(classMeta, hotClassMetaSet);
  }
  WriteProfileData<MetaItem>(classMeta, kClassMeta);

  MapleProfile<MetaItem> methodMeta;
  {
    std::lock_guard<std::mutex> lock(methodMetaMtx);
    GenMetaData(methodMeta, hotMethodMetaSet);
  }
  WriteProfileData<MetaItem>(methodMeta, kMethodMeta);

  MapleProfile<MetaItem> fieldMeta;
  {
    std::lock_guard<std::mutex> lock(fieldMetaMtx);
    GenMetaData(fieldMeta, hotFieldMetaSet);
  }
  WriteProfileData<MetaItem>(fieldMeta, kFieldMeta);

  // process reflection str profile data;
  MapleProfile<ReflectionStrItem> reflectStrMeta;
  {
    std::lock_guard<std::mutex> lock(reflectStrMtx);
    GenReflectStrMetaData(reflectStrMeta, hotRefectionStrInfo);
  }
  WriteProfileData<ReflectionStrItem>(reflectStrMeta, kReflectionStr);

  MapleProfile<MethodSignatureItem> methodSignature;
  {
    std::lock_guard<std::mutex> lock(methodSignatureMtx);
    GenMethodSignatureData(methodSignature, hotMethodSignatureSet);
  }
  WriteProfileData<MethodSignatureItem>(methodSignature, kMethodSig);
}

void ProfileWriter::ProcessFuncProf() {
  // when get the profile data of metdata and function will case some reflection str recored
  // so first disble meta profile
  MRT_DisableMetaProfile();
  std::unordered_map<std::string, std::vector<FuncProfInfo>> funcProfileRaw;
  MapleIRProf funcIRProfRaw;
  MapleProfile<FunctionItem> funcProfile;
  LinkerAPI::Instance().DumpAllMplFuncProfile(funcProfileRaw);
  LinkerAPI::Instance().DumpAllMplFuncIRProfile(funcIRProfRaw);
  MRT_EnableMetaProfile();
  GenFuncData(funcProfile, funcProfileRaw);
  WriteProfileData<FunctionItem>(funcProfile, kFunction);
  // write func ir prof
  WriteIRProfData(funcIRProfRaw);
}
// some kind prof data save the raw data in profile directly like literal and BB profile
// because raw prof data is readable,and compact
void ProfileWriter::ProcessRawProf() {
  // process literal string
  std::stringstream literalProfile;
  std::string literalContent;
  DumpConstStringPool(literalProfile, true);
  literalContent = literalProfile.str();
  uint32_t literalContentSize = static_cast<uint32_t>(literalContent.size());
  if (literalContentSize != 0) {
    profileHeaders.emplace_back(profileDataSize, kLiteral, 0);
    profileDataNum++;
    profileDataSize += literalContentSize;
    (void)profileData.insert(profileData.end(), literalContent.c_str(), literalContent.c_str() + literalContentSize);
  }
  // process BB profile
  std::ostringstream bbProfile;
  LinkerAPI::Instance().DumpBBProfileInfo(bbProfile);
  std::string bbProfileContent = bbProfile.str();
  uint32_t bbProfileContentSize = static_cast<uint32_t>(bbProfileContent.size());
  if (bbProfileContentSize != 0) {
    profileHeaders.emplace_back(profileDataSize, kBBInfo, 0);
    profileDataNum++;
    profileDataSize += bbProfileContentSize;
    (void)profileData.insert(profileData.end(), bbProfileContent.c_str(),
                             bbProfileContent.c_str() + bbProfileContentSize);
  }
}

void ProfileWriter::RemoveProfile() {
  if (std::remove(path.c_str())) {
    if (errno != ENOENT) {
      LOG(ERROR) << "RemoveProfile failed to remove " << path << ", " << strerror(errno);
    }
    return;
  }
  LOG(INFO) << "RemoveProfile remove " << path << " successfully";
  return;
}

void ProfileWriter::Init() {
  if (!isSystemServer) {
    char *str = nullptr;
    std::string packageName = LinkerAPI::Instance().GetAppPackageName();
    uint32_t packageNameIdx = GetorInsertStrInTab(packageName);
    profileHeaders.emplace_back(profileDataSize, kFileDesc, 1);
    profileDataNum++;
    str = reinterpret_cast<char*>(&packageNameIdx);
    profileData.insert(profileData.end(), str, str + sizeof(packageNameIdx));
    profileDataSize  = profileDataSize + sizeof(packageNameIdx);
  }
}

void ProfileWriter::ProcessProfData() {
  uint64_t processProfileStart = timeutils::NanoSeconds();
  ProcessMetaData();
  ProcessFuncProf();
  ProcessRawProf();
  uint64_t processProfileEnd = timeutils::NanoSeconds();
  uint64_t processProfileCost = processProfileEnd - processProfileStart;
  LOG(INFO) << "Total process Profile time: " << Pretty(processProfileCost / nsPerUs) << "us" << maple::endl;
}

void ProfileWriter::InitFileHeader(Header &header) {
  std::copy_n(kProfileMagic, sizeof(kProfileMagic), header.magic);
  std::copy_n(kVer, sizeof(kVer), header.ver);
  header.profileNum = profileDataNum;
  if (!isSystemServer) {
    header.profileFileType = kApp;
  } else {
    header.profileFileType = kSystemServer;
  }
  uint32_t headerSize = sizeof(Header) + (profileDataNum - 1) * sizeof(ProfileDataInfo);
  // adjust the offset
  header.stringTabOff = headerSize + profileDataSize;
  header.stringCount = static_cast<uint32_t>(strTab.size());
  for (auto &item : profileHeaders) {
    item.profileDataOff = item.profileDataOff + headerSize;
  }
}

void ProfileWriter::WriteFormatDataToFile() {
  uint64_t processProfileEnd = timeutils::NanoSeconds();
  Header header;
  InitFileHeader(header);
  bool res = true;
  bool removeIfFailed = true;
  std::vector<char> strData;
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (errno != EACCES) {
      LOG(ERROR) << "SaveProfile failed to open " << path << ", " << strerror(errno);
    } else {
      removeIfFailed = false;
    }
    res = false;
    goto END;
  }
  // write header
  if (!out.write(reinterpret_cast<char*>(&header), sizeof(Header) - sizeof(ProfileDataInfo))) {
    LOG(ERROR) << "SaveProfile  failed to write header for " << path << ", " << strerror(errno);
    res = false;
    goto END;
  }
  // write profile date info
  if (!out.write(reinterpret_cast<char*>(profileHeaders.data()), profileHeaders.size() * sizeof(ProfileDataInfo))) {
    LOG(ERROR) << "SaveProfile  failed to write ProfileDataInfo for " << path << ", " << strerror(errno);
    res = false;
    goto END;
  }
  // write profile date
  if (!out.write(reinterpret_cast<char*>(profileData.data()), profileData.size())) {
    LOG(ERROR) << "SaveProfile  failed to write ProfileData for " << path << ", " << strerror(errno);
    res = false;
    goto END;
  }
  GenStrData(strData);
  // write strTab
  if (!out.write(reinterpret_cast<char*>(strData.data()), strData.size())) {
    LOG(ERROR) << "SaveProfile  failed to write strData for " << path << ", " << strerror(errno);
    res = false;
    goto END;
  }

END:
  LOG(INFO) << "SaveProfile res=" << res;
  if (!res && removeIfFailed) {
    LOG(INFO) << "SaveProfile to remove ";
    RemoveProfile(); // In case of more exceptions, from here to re-save the cache.
  }
  uint64_t saveProfileEnd = timeutils::NanoSeconds();
  uint64_t saveProfileCost = saveProfileEnd - processProfileEnd;
  LOG(INFO) << "Total save Profile time: " << Pretty(saveProfileCost / nsPerUs) << "us" << maple::endl;
}

void ProfileWriter::WriteToFile() {
  Init();
  ProcessProfData();
  if (!profileDataNum) {
    LOG(INFO) << "dump profile no data for save" << maple::endl;
    return ;
  }
  WriteFormatDataToFile();
}

extern "C" void MRT_SaveProfile(const std::string &path, bool isSystemServer) {
  ProfileWriter profileWriter(path, isSystemServer);
  profileWriter.WriteToFile();
}

void MCC_SaveProfile() {
  LOG(INFO) << "SaveProfile starting...";
  std::string saveName = "/data/anr/maple_all.prof";
  MRT_SaveProfile(saveName, false);
}
} // namespace maplert
