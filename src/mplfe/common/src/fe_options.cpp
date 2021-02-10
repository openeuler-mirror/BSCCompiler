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
#include "fe_options.h"
#include "fe_file_type.h"
namespace maple {
FEOptions FEOptions::options;
FEOptions::FEOptions()
    : isGenMpltOnly(false),
      isGenAsciiMplt(false),
      outputPath(""),
      outputName(""),
      dumpLevel(kDumpLevelDisable),
      isDumpTime(false),
      nthreads(0),
      dumpThreadTime(false) {}

void FEOptions::AddInputClassFile(const std::string &fileName) {
  FEFileType::FileType type = FEFileType::GetInstance().GetFileTypeByMagicNumber(fileName);
  if (type == FEFileType::FileType::kClass) {
    inputClassFiles.push_back(fileName);
  } else {
    WARN(kLncWarn, "invalid input class file %s...skipped", fileName.c_str());
  }
}

void FEOptions::AddInputJarFile(const std::string &fileName) {
  FEFileType::FileType type = FEFileType::GetInstance().GetFileTypeByMagicNumber(fileName);
  if (type == FEFileType::FileType::kJar) {
    inputJarFiles.push_back(fileName);
  } else {
    WARN(kLncWarn, "invalid input jar file %s...skipped", fileName.c_str());
  }
}

void FEOptions::AddInputDexFile(const std::string &fileName) {
  FEFileType::FileType type = FEFileType::GetInstance().GetFileTypeByMagicNumber(fileName);
  if (type == FEFileType::FileType::kDex) {
    inputDexFiles.push_back(fileName);
  } else {
    WARN(kLncWarn, "invalid input dex file %s...skipped", fileName.c_str());
  }
}

}  // namespace maple