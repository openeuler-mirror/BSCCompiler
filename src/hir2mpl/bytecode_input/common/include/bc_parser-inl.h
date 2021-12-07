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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_PARSER_INL_H_
#define HIR2MPL_BC_INPUT_INCLUDE_BC_PARSER_INL_H_
#include "bc_parser.h"

namespace maple {
namespace bc {
template<class T>
BCParser<T>::BCParser(uint32 fileIdxIn, const std::string &fileNameIn, const std::list<std::string> &classNamesIn)
    : BCParserBase(fileIdxIn, fileNameIn, classNamesIn), reader(std::make_unique<T>(fileIdxIn, fileNameIn)) {}

template<class T>
bool BCParser<T>::OpenFileImpl() {
  return reader->OpenAndMap();
}

template<class T, class V>
int MethodProcessTask<T, V>::RunImpl(MplTaskParam *param) {
  (void)param;
  parser.ProcessDexClassMethod(klass, isVirtual, index, idxPair);
  return 0;
}

template<class T, class V>
int MethodProcessTask<T, V>::FinishImpl(MplTaskParam *param) {
  (void)param;
  return 0;
}

template<class T, class V>
void MethodProcessSchedular<T, V>::AddMethodProcessTask(T &parser,
    std::unique_ptr<V> &klass, bool isVirtual, uint32 index, std::pair<uint32, uint32> &idxPair) {
  std::unique_ptr<MethodProcessTask<T, V>> task =
      std::make_unique<MethodProcessTask<T, V>>(parser, klass, isVirtual, index, idxPair);
  AddTask(task.get());
  tasks.push_back(std::move(task));
}

template<class T, class V>
void MethodProcessSchedular<T, V>::CallbackThreadMainStart() {
  std::thread::id tid = std::this_thread::get_id();
  if (FEOptions::GetInstance().GetDumpLevel() >= FEOptions::kDumpLevelInfoDebug) {
    INFO(kLncInfo, "Start Run Thread (tid=%lx)", tid);
  }
  FEConfigParallel::GetInstance().RegisterRunThreadID(tid);
}
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_BC_PARSER_INL_H_