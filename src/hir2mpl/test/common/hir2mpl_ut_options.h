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
#ifndef HIR2MPL_INCLUDE_HIR2MPL_UT_OPTIONS_H
#define HIR2MPL_INCLUDE_HIR2MPL_UT_OPTIONS_H
#include <string>
#include <list>
#include "driver_option_common.h"

namespace maple {
class HIR2MPLUTOptions : public maple::MapleDriverOptionBase {
 public:
  HIR2MPLUTOptions();
  ~HIR2MPLUTOptions() = default;
  void DumpUsage() const;
  bool SolveArgs(int argc, char **argv);
  template <typename Out>
  static void Split(const std::string &s, char delim, Out result);

  static HIR2MPLUTOptions &GetInstance() {
    static HIR2MPLUTOptions options;
    return options;
  }

  bool GetRunAll() const {
    return runAll;
  }

  bool GetRunAllWithCore() const {
    return runAllWithCore;
  }

  bool GetGenBase64() const {
    return genBase64;
  }

  std::string GetBase64SrcFileName() const {
    return base64SrcFileName;
  }

  std::string GetCoreMpltName() const {
    return coreMpltName;
  }

  const std::list<std::string> &GetClassFileList() const {
    return classFileList;
  }

  const std::list<std::string> &GetJarFileList() const {
    return jarFileList;
  }

  const std::list<std::string> &GetMpltFileList() const {
    return mpltFileList;
  }

 private:
  bool runAll;
  bool runAllWithCore;
  bool genBase64;
  std::string base64SrcFileName;
  std::string coreMpltName;
  std::list<std::string> classFileList;
  std::list<std::string> jarFileList;
  std::list<std::string> mpltFileList;
};
}  // namespace maple

#endif  // HIR2MPL_INCLUDE_HIR2MPL_UT_OPTIONS_H