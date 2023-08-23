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
#include "linker/linker_debug.h"

#include "linker/linker_model.h"
#include "linker/linker_inline.h"
#include "file_layout.h"
#include "collector/cp_generator.h"
#include "interp_support.h"
using namespace maple;

namespace maplert {
using namespace linkerutils;
FeatureName Debug::featureName = kFDebug;
// Dump all the section info of all loaded maple-so
void Debug::DumpAllMplSectionInfo(std::ostream &os) {
  uint64_t sumMethodsHotSize = 0;
  uint64_t sumMethodsColdSize = 0;
  uint64_t sumMethodsCompactSize = 0;
  uint64_t sumFieldsHotSize = 0;
  uint64_t sumFieldsColdSize = 0;
  uint64_t sumFieldsCompactSize = 0;
  uint64_t sumVtbHotSize = 0;
  uint64_t sumVtbColdSize = 0;
  uint64_t sumItbHotSize = 0;
  uint64_t sumItbColdSize = 0;
  uint64_t sumSuperClassHotSize = 0;
  uint64_t sumSuperClassColdSize = 0;
  uint64_t sumClassMeta = 0;
  uint64_t sumHotLiteralSize = 0;
  uint64_t sumColdLiteralSize = 0;
  uint64_t sumEhframeSize = 0;
  uint64_t sumReflectionStrSize = 0;
  uint64_t sumMuidTabSize = 0;
  void *symAddr = nullptr;
  auto handle = [&, this](LinkerMFileInfo &mplInfo) {
    os << mplInfo.name << ":\n";
    void *javaStartAddr = mplInfo.GetTblBegin(kJavaText);
    void *javaEndAddr = reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(javaStartAddr) + mplInfo.GetTblSize(kJavaText));
    os << "\tjava-text-start:" << std::hex << javaStartAddr << "\n";
    os << "\tjava-text-end:" << std::hex << javaEndAddr << "\n";
    os << "\tstrtab-start:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetStrTabBegin", true) << "\n";
    os << "\tstrtab-end:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetStrTabEnd", true) << "\n";
    os << "\tjnitab-start:" << std::hex << GetSymbolAddr(mplInfo.handle, "__MRT_GetJNITable_Begin", true) << "\n";
    os << "\tjnitab-end:" << std::hex << GetSymbolAddr(mplInfo.handle, "__MRT_GetJNITable_End", true) << "\n";
    os << "\tjnifunctab-start:" << std::hex << GetSymbolAddr(mplInfo.handle, "__MRT_GetJNIFuncTable_Begin", true) <<
        "\n";
    os << "\tjnifunctab-end:" << std::hex << GetSymbolAddr(mplInfo.handle, "__MRT_GetJNIFuncTable_End", true) << "\n";
    os << "\trangetab-start:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetRangeTableBegin", true) << "\n";
    os << "\trangetab-end:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetRangeTableEnd", true) << "\n";

    os << "\treflectColdStr_begin:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetColdStrTabBegin", true) <<
        "\n";
    uint64_t reflectionColdStrSize =
        static_cast<uint64_t>(static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetColdStrTabEnd", true)) -
        static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetColdStrTabBegin", true)));
    os << "\treflectStartHotStr_begin:" << std::hex <<
        GetSymbolAddr(mplInfo.handle, "MRT_GetStartHotStrTabBegin", true) << "\n";
    uint64_t reflectionStartHotStrSize =
        static_cast<uint64_t>(static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetStartHotStrTabEnd", true)) -
        static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetStartHotStrTabBegin", true)));
    os << "\treflectBothHotStr_begin:" << std::hex <<
        GetSymbolAddr(mplInfo.handle, "MRT_GetBothHotStrTabBegin", true) << "\n";
    uint64_t reflectionBothHotStrSize =
        static_cast<uint64_t>(static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetBothHotStrTabEnd", true)) -
        static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetBothHotStrTabBegin", true)));
    os << "\treflectRunHotStr_begin:" << std::hex <<
        GetSymbolAddr(mplInfo.handle, "MRT_GetRunHotStrTabBegin", true) << "\n";
    uint64_t reflectionRunHotStrSize =
        static_cast<uint64_t>(static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetRunHotStrTabEnd", true)) -
        static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetRunHotStrTabBegin", true)));
    sumReflectionStrSize += (reflectionColdStrSize + reflectionStartHotStrSize +
                             reflectionBothHotStrSize + reflectionRunHotStrSize);
    os << "\treflectColdStr size:" << std::hex << reflectionColdStrSize << "\n";
    os << "\treflectStartHotStr size:" << std::hex << reflectionStartHotStrSize << "\n";
    os << "\treflectBothHotStr size:" << std::hex << reflectionBothHotStrSize << "\n";
    os << "\treflectRunHotStr size:" << std::hex << reflectionRunHotStrSize << "\n";

    os << "\tmuidTab_begin:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetMuidTabBegin", true) << "\n";
    uint64_t muidTabSize =
        static_cast<uint64_t>(static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetMuidTabEnd", true)) -
        static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetMuidTabBegin", true)));
    sumMuidTabSize += muidTabSize;
    os << "\tmuidTab size:" << std::hex << muidTabSize << "\n";

    os << "\tEhframe_begin:" << std::hex << GetSymbolAddr(mplInfo.handle, "MRT_GetEhframeStart", true) << "\n";
    uint64_t ehframeSize =
        static_cast<uint64_t>(static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetEhframeEnd", true)) -
        static_cast<char*>(GetSymbolAddr(mplInfo.handle, "MRT_GetEhframeStart", true)));
    sumEhframeSize += ehframeSize;
    os << "\tEhframe size:" << std::hex << ehframeSize << "\n";
    for (uint32_t i = 0; i < static_cast<uint32_t>(kLayoutTypeCount); ++i) {
      std::string markerName = "__MBlock_" + GetLayoutTypeString(i) + "_func_start";
      symAddr = GetSymbolAddr(mplInfo.handle, markerName.c_str(), false);
      if (symAddr != nullptr) {
        os << "\t" << markerName << ":" << std::hex << *(reinterpret_cast<int64_t*>(symAddr)) << "\n";
      }
    }
    if ((symAddr = GetSymbolAddr(mplInfo.handle, "__MBlock_globalVars_hot_begin", false))) {
      os << "\t__MBlock_globalVars_hot_begin:" << std::hex << *(reinterpret_cast<int64_t*>(symAddr)) << "\n";
    }
    if ((symAddr = GetSymbolAddr(mplInfo.handle, "__MBlock_globalVars_cold_begin", false))) {
      os << "\t__MBlock_globalVars_cold_begin:" << std::hex << *(reinterpret_cast<int64_t*>(symAddr)) << "\n";
    }
    if ((symAddr = GetSymbolAddr(mplInfo.handle, "__MBlock_globalVars_cold_end", false))) {
      os << "\t__MBlock_globalVars_cold_end:" << std::hex << *(reinterpret_cast<int64_t*>(symAddr)) << "\n";
    }

    sumHotLiteralSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock_literal_hot");
    sumColdLiteralSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock_literal_cold");
    sumClassMeta += mplInfo.GetTblSize(kTabClassMetadata);

    sumMethodsHotSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__methods_info__hot");
    sumMethodsColdSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__methods_info__cold");
    sumMethodsCompactSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__methods_infocompact__cold");

    sumFieldsHotSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__fields_info__hot");
    sumFieldsColdSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__fields_info__cold");
    sumFieldsCompactSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__fields_infocompact__cold");

    sumVtbHotSize += mplInfo.GetTblSize(kVTable);
    sumVtbColdSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__vtb_cold");
    sumItbHotSize += mplInfo.GetTblSize(kITable);
    sumItbColdSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__itb_cold");
    sumSuperClassHotSize += mplInfo.GetTblSize(kDataSuperClass);
    sumSuperClassColdSize += DumpMetadataSectionSize(os, mplInfo.handle, "__MBlock__superclasses__cold");
  };
  pInvoker->mplInfoList.ForEach(handle);
  os << std::dec;
  os << "All maple*.so literal information: " << "\n";
  os << "\tsumLiteralHotSize: " << sumHotLiteralSize << "\n";
  os << "\tsumLiteralColdSize: " << sumColdLiteralSize << "\n";

  os << "All maple*.so MetaData information: " << "\n";
  os << "\tsumClassMetaSize: " << sumClassMeta << "\n";
  os << "\tsumMethodsHotSize: " << sumMethodsHotSize << "\n";
  os << "\tsumMethodsColdSize: " << sumMethodsColdSize << "\n";
  os << "\tsumMethodsCompactSize: " << sumMethodsCompactSize << "\n";
  os << "\tsumFieldsHotSize: " << sumFieldsHotSize << "\n";
  os << "\tsumFieldsColdSize: " << sumFieldsColdSize << "\n";
  os << "\tsumFieldsCompactSize: " << sumFieldsCompactSize << "\n";
  os << "\tsumVtbHotSize: " << sumVtbHotSize << "\n";
  os << "\tsumVtbColdSize: " << sumVtbColdSize << "\n";
  os << "\tsumItbHotSize: " << sumItbHotSize << "\n";
  os << "\tsumItbColdSize: " << sumItbColdSize << "\n";
  os << "\tsumSuperClassHotSize: " << sumSuperClassHotSize << "\n";
  os << "\tsumSuperClassColdSize: " << sumSuperClassColdSize << "\n";
  os << "\tsumEhframeSize: " << sumEhframeSize << "\n";
  os << "\tsumMuidTabSize: " << sumMuidTabSize << "\n";
  os << "\tsumReflectionStrSize: " << sumReflectionStrSize << "\n";
}

// Dump all the function profile of all loaded maple-so
void Debug::DumpAllMplFuncProfile(std::unordered_map<std::string, std::vector<FuncProfInfo>> &funcProfileRaw) {
  size_t size;
  LinkerFuncProfileTableItem *tableItem = nullptr;
  auto handle = [&](LinkerMFileInfo &mplInfo) {
    size = mplInfo.GetTblSize(kMethodProfile);
    tableItem = mplInfo.GetTblBegin<LinkerFuncProfileTableItem*>(kMethodProfile);
    VLOG(profiler) << mplInfo.name << " size " << size << "\n";
    bool emptySize = (!size);
    if (emptySize) {
      return;
    }
    auto &profileList = funcProfileRaw[mplInfo.name];
    LinkerInfTableItem *pInfTable = mplInfo.GetTblBegin<LinkerInfTableItem*>(kMethodInfo);
    for (size_t i = 0; i < size; ++i) {
      LinkerFuncProfileTableItem item = tableItem[i];
      if (item.callTimes) {
        LinkerInfTableItem *infItem = reinterpret_cast<LinkerInfTableItem*>(pInfTable + i);
        std::string funcName = GetMethodSymbolByOffset(*infItem);
        auto startupProfile = mplInfo.funProfileMap.find(static_cast<uint32_t>(i));
        uint8_t layoutType = static_cast<uint8_t>(kLayoutBootHot);
        if (mplInfo.funProfileMap.empty()) {
          layoutType = static_cast<uint8_t>(kLayoutBootHot);
        } else if (startupProfile == mplInfo.funProfileMap.end()) {
          layoutType = static_cast<uint8_t>(kLayoutRunHot);
        } else if ((startupProfile->second).callTimes == item.callTimes) {
          layoutType = static_cast<uint8_t>(kLayoutBootHot);
        } else {
          layoutType = static_cast<uint8_t>(kLayoutBothHot);
        }
        profileList.emplace_back(item.callTimes, layoutType, funcName);
      }
    }
  };
  pInvoker->mplInfoList.ForEach(handle);
}

// Dump the method undefine table of 'handle'.
bool Debug::DumpMethodUndefSymbol(LinkerMFileInfo &mplInfo) {
  size_t size = mplInfo.GetTblSize(kMethodUndef);
  if (size == 0) {
    LINKER_DLOG(mpllinker) << "failed, size is zero in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerAddrTableItem *pTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodUndef);
  if (pTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pTable is null in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodUndefMuid);
  if (pMuidTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pMuidTable is null in " << mplInfo.name << maple::endl;
    return false;
  }
  for (size_t i = 0; i < size; ++i) {
    LinkerAddrTableItem item = pTable[i];
    LinkerMuidTableItem muidItem = pMuidTable[i];
    if (item.addr == 0) {
      LINKER_LOG(INFO) << "(" << i << "), \tMUID = " << muidItem.muid.ToStr() << " in " << mplInfo.name << maple::endl;
    }
    LINKER_DLOG(mpllinker) << "(" << i << "), \taddr = " << item.addr << ", MUID = " << muidItem.muid.ToStr() <<
        " in " << mplInfo.name << maple::endl;
  }
  return true;
}

// Dump the method define table of 'handle'.
bool Debug::DumpMethodSymbol(LinkerMFileInfo &mplInfo) {
  size_t size = mplInfo.GetTblSize(kMethodDef);
  if (size == 0) {
    LINKER_DLOG(mpllinker) << "failed, size is zero in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerAddrTableItem *pTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kMethodDefOrig);
  if (pTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pTable is null in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kMethodDefMuid);
  if (pMuidTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pMuidTable is null in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerInfTableItem *pInfTable = mplInfo.GetTblBegin<LinkerInfTableItem*>(kMethodInfo);
  if (pInfTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pInfTable is null in " << mplInfo.name << maple::endl;
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    LinkerAddrTableItem *item = reinterpret_cast<LinkerAddrTableItem*>(pTable + i);
    LinkerMuidTableItem *muidItem = reinterpret_cast<LinkerMuidTableItem*>(pMuidTable + i);
    LinkerInfTableItem *infItem = reinterpret_cast<LinkerInfTableItem*>(pInfTable + i);
    LINKER_LOG(INFO) << "(" << i << "), \tMUID=" << muidItem->muid.ToStr() << " in " << mplInfo.name << maple::endl;
    LINKER_LOG(INFO) << "(" << i << "), \taddr=" << item->addr << " in " << mplInfo.name << maple::endl;
    LINKER_LOG(INFO) << "(" << i << "), \tsize=" << infItem->size << " in " << mplInfo.name << maple::endl;
    LINKER_LOG(INFO) << "(" << i << "), \tsym=" << GetMethodSymbolByOffset(*infItem) << " in " <<
        mplInfo.name << maple::endl;
  }
  return true;
}

// Dump the data undefine table of 'handle'.
bool Debug::DumpDataUndefSymbol(LinkerMFileInfo &mplInfo) {
  size_t size = mplInfo.GetTblSize(kDataUndef);
  if (size == 0) {
    LINKER_LOG(ERROR) << "failed, size is zero, in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerAddrTableItem *pTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataUndef);
  if (pTable == nullptr) {
    LINKER_LOG(ERROR) << "failed, pTable is null, in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataUndefMuid);
  if (pMuidTable == nullptr) {
    LINKER_LOG(ERROR) << "failed, pMuidTable is null in " << mplInfo.name << maple::endl;
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    LinkerAddrTableItem item = pTable[i];
    LinkerMuidTableItem muidItem = pMuidTable[i];
    if (item.addr == 0) {
      LINKER_LOG(INFO) << "(" << i << "), \tMUID=" << muidItem.muid.ToStr() << " in " << mplInfo.name << maple::endl;
    }
    LINKER_DLOG(mpllinker) << "(" << i << "), \taddr=" << item.addr << ", MUID=" << muidItem.muid.ToStr() << " in " <<
        mplInfo.name << maple::endl;
  }
  return true;
}

// Dump the data define table of 'handle'.
bool Debug::DumpDataSymbol(LinkerMFileInfo &mplInfo) {
  size_t size = mplInfo.GetTblSize(kDataDef);
  if (size == 0) {
    LINKER_DLOG(mpllinker) << "failed, size is zero, in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerAddrTableItem *pTable = mplInfo.GetTblBegin<LinkerAddrTableItem*>(kDataDefOrig);
  if (pTable == nullptr) {
    LINKER_DLOG(mpllinker) << "failed, pTable is null, in " << mplInfo.name << maple::endl;
    return false;
  }
  LinkerMuidTableItem *pMuidTable = mplInfo.GetTblBegin<LinkerMuidTableItem*>(kDataDefMuid);
  if (pMuidTable == nullptr) {
    LINKER_LOG(ERROR) << "failed, pMuidTable is null in " << mplInfo.name << maple::endl;
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    LinkerAddrTableItem item = pTable[i];
    LinkerMuidTableItem muidItem = pMuidTable[i];
    LINKER_LOG(INFO) << "(" << i << "), \tMUID=" << muidItem.muid.ToStr() << " in " << mplInfo.name << maple::endl;
    LINKER_LOG(INFO) << "(" << i << "), \taddr=" << item.addr << " in " << mplInfo.name << maple::endl;
  }
  return true;
}

// Lazy-Binding routines start
void Debug::DumpStackInfoInLog() {
  // We needn't care the performance of unwinding here.
  std::vector<UnwindContext> uwContextStack;
  // Unwind as many as possible till reaching the end.
  MapleStack::FastRecordCurrentJavaStack(uwContextStack, MAPLE_STACK_UNWIND_STEP_MAX);

  for (auto &uwContext : uwContextStack) {
    if (!UnwindContextInterpEx::TryDumpStackInfoInLog(*pInvoker, uwContext)) {
      if (uwContext.IsCompiledContext()) {
        std::string methodName;
        uwContext.frame.GetJavaMethodFullName(methodName);
        LinkerMFileInfo *mplInfo =
            pInvoker->GetLinkerMFileInfo(kFromAddr, reinterpret_cast<const void*>(uwContext.frame.ip));
        if (mplInfo != nullptr) {
          LINKER_LOG(INFO) << methodName << ":" << uwContext.frame.ip << ":" << mplInfo->name << maple::endl;
        } else {
          LINKER_LOG(INFO) << methodName << ":" << uwContext.frame.ip << ":NO_SO_NAME" << maple::endl;
        }
      }
    }
  }
}

uint64_t Debug::DumpMetadataSectionSize(std::ostream &os, void *handle, const std::string sectionName) {
  std::ios::fmtflags f(os.flags());
  std::string hotOrCold = (sectionName.find("hot") != std::string::npos) ? "hot" : "cold";
  std::string startString = sectionName + "_begin";
  std::string endString = sectionName + "_end";
  void *end = GetSymbolAddr(handle, endString.c_str(), false);
  void *start = GetSymbolAddr(handle, startString.c_str(), false);
  uint64_t size = static_cast<uint64_t>(reinterpret_cast<char*>(end) - reinterpret_cast<char*>(start));
  os << "\t" << startString << ": " << std::hex << start << "\n";
  os << "\t" << hotOrCold << " " << sectionName << " size: " << size << "\n";
  os.flags(f);
  return size;
}

void Debug::DumpBBProfileInfo(std::ostream &os) {
  auto handle = [&os](const LinkerMFileInfo &mplInfo) {
    void *bbProfileTabStart = GetSymbolAddr(mplInfo.handle, kBBProfileTabBegin, true);
    void *bbProfileTabEnd = GetSymbolAddr(mplInfo.handle, kBBProfileTabEnd, true);
    size_t itemSize = (reinterpret_cast<size_t>(bbProfileTabEnd) -
        reinterpret_cast<size_t>(bbProfileTabStart)) / sizeof(uint32_t);

    char *bbProfileStrTabStart = static_cast<char*>(GetSymbolAddr(mplInfo.handle, kBBProfileStrTabBegin, true));
    char *bbProfileStrTabEnd = reinterpret_cast<char*>(GetSymbolAddr(mplInfo.handle, kBBProfileStrTabEnd, true));
    std::vector<std::string> profileStr;
    std::string str(bbProfileStrTabStart, bbProfileStrTabEnd - bbProfileStrTabStart);
    std::stringstream ss;
    ss.str(str);
    std::string item;
    while (std::getline(ss, item, '\0')) {
      profileStr.emplace_back(item);
    }
    if (profileStr.size() != itemSize) {
      LOG(INFO) << "profileStr size " << profileStr.size() << " doestn't match item size " <<
          itemSize << " in " << mplInfo.name << maple::endl;
      LOG(INFO) << "profile Tab Start " << std::hex <<  bbProfileTabStart << " end " <<
          bbProfileTabEnd << std::dec << maple::endl;
      return;
    }
    uint32_t *bbProfileTab = reinterpret_cast<uint32_t*>(bbProfileTabStart);
    for (size_t i = 0; i < itemSize; ++i) {
      os << profileStr[i] << ":" << bbProfileTab[i] << "\n";
    }
  };
  pInvoker->mplInfoList.ForEach(handle);
}
// Dump all the func IR  profile of all loaded maple-so
void Debug::DumpAllMplFuncIRProfile(std::unordered_map<std::string, MFileIRProfInfo> &funcProfileRaw) {
  auto handle = [&](LinkerMFileInfo &mplInfo) {
    auto counterSize = mplInfo.GetTblSize(kFuncIRProfCounter);
    auto counterTable = mplInfo.GetTblBegin<LinkerFuncProfileTableItem*>(kFuncIRProfCounter);
    auto descSize = mplInfo.GetTblSize(kFuncIRProfDesc);
    auto descTable = mplInfo.GetTblBegin<LinkerFuncProfDescItem*>(kFuncIRProfDesc);
    VLOG(profiler) << mplInfo.name << " counter size " << counterSize << " desc size " << descSize << "\n";
    if (!counterSize || !descSize) {
      return;
    }
    auto &profileList = funcProfileRaw[mplInfo.name];
    auto &descTab = profileList.descTab;
    auto &counterTab = profileList.counterTab;
    counterTab.reserve(counterSize);
    LinkerInfTableItem *pInfTable = mplInfo.GetTblBegin<LinkerInfTableItem*>(kMethodInfo);
    for (size_t i = 0; i < descSize; ++i) {
      LinkerFuncProfDescItem descItem = descTable[i];
      if ((descItem.start >= counterSize) || (descItem.end >= counterSize)) {
        return;
      }
      LinkerInfTableItem *infItem = reinterpret_cast<LinkerInfTableItem*>(pInfTable + i);
      std::string funcName = GetMethodSymbolByOffset(*infItem);
      VLOG(profiler) << funcName << " counter range [" << descItem.start << "," << descItem.end << "]" << "\n";
      descTab.emplace_back(funcName, descItem.hash, descItem.start, descItem.end);
    }

    for (size_t i = 0; i < counterSize; ++i) {
      LinkerFuncProfileTableItem counterItem = counterTable[i];
      counterTab.push_back(counterItem.callTimes);
    }
  };
  pInvoker->mplInfoList.ForEach(handle);
}
} // namespace maplert
