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
#ifndef HIR2MPL_INCLUDE_JBC_INPUT_H
#define HIR2MPL_INCLUDE_JBC_INPUT_H
#include <string>
#include <list>
#include "mempool_allocator.h"
#include "jbc_class.h"

namespace maple {
namespace jbc {
class JBCInput {
 public:
  explicit JBCInput(MIRModule &moduleIn);
  ~JBCInput();
  void ReleaseMemPool();
  bool ReadClassFile(const std::string &fileName);
  bool ReadClassFiles(const std::list<std::string> &fileNames);
  bool ReadJarFile(const std::string &fileName);
  bool ReadJarFiles(const std::list<std::string> &fileNames);
  const JBCClass *GetFirstClass();
  const JBCClass *GetNextClass();
  void RegisterSrcFileInfo(JBCClass &klass);

  const MIRModule &GetModule() const {
    return module;
  }

 protected:
  MIRModule &module;
  MemPool *mp;
  MapleAllocator allocator;
  MapleList<JBCClass*> klassList;

 private:
  MapleList<JBCClass*>::const_iterator itKlass;
};
}  // namespace jbc
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_JBC_INPUT_H