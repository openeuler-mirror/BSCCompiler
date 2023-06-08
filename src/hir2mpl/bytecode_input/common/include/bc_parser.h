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
#ifndef MPL_FE_BC_INPUT_BC_PARSER_H
#define MPL_FE_BC_INPUT_BC_PARSER_H
#include <memory>
#include <list>
#include <string>
#include "bc_io.h"
#include "bc_class.h"
#include "bc_parser_base.h"
#include "mpl_scheduler.h"
#include "fe_options.h"

namespace maple {
namespace bc {
template <class T>
class BCParser : public BCParserBase {
 public:
  BCParser(uint32 fileIdxIn, const std::string &fileNameIn, const std::list<std::string> &classNamesIn);
  ~BCParser() override = default;

 protected:
  bool OpenFileImpl() override;
  uint32 CalculateCheckSumImpl(const uint8 *data, uint32 size) override = 0;
  bool ParseHeaderImpl() override = 0;
  bool VerifyImpl() override = 0;
  bool RetrieveIndexTables() override = 0;
  bool RetrieveUserSpecifiedClasses(std::list<std::unique_ptr<BCClass>> &klasses) override = 0;
  bool RetrieveAllClasses(std::list<std::unique_ptr<BCClass>> &klasses) override = 0;
  bool CollectAllDepTypeNamesImpl(std::unordered_set<std::string> &depSet) override = 0;
  bool CollectMethodDepTypeNamesImpl(std::unordered_set<std::string> &depSet, BCClassMethod &bcMethod)
      const override = 0;
  bool CollectAllClassNamesImpl(std::unordered_set<std::string> &classSet) override = 0;

  std::unique_ptr<T> reader;
};

template <class T, class V>
class MethodProcessTask : public MplTask {
 public:
  MethodProcessTask(T &argParser, std::unique_ptr<V> &argKlass,
      bool argIsVirtual, uint32 index, std::pair<uint32, uint32> argIdxPair)
      : isVirtual(argIsVirtual),
        index(index),
        klass(argKlass),
        idxPair(argIdxPair),
        parser(argParser) {}
  ~MethodProcessTask() override = default;

 protected:
  int RunImpl(MplTaskParam *param) override;
  int FinishImpl(MplTaskParam *param) override;

 private:
  bool isVirtual;
  uint32 index;
  std::unique_ptr<V> &klass;
  std::pair<uint32, uint32> idxPair;
  T &parser;
};

template <class T, class V>
class MethodProcessSchedular : public MplScheduler {
 public:
  explicit MethodProcessSchedular(const std::string &name)
      : MplScheduler(name) {}
  ~MethodProcessSchedular() override {
    FEConfigParallel::GetInstance().RunThreadIDCleanUp();
  }

  void AddMethodProcessTask(T &parser, std::unique_ptr<V> &klass,
      bool isVirtual, uint32 index, std::pair<uint32, uint32> &idxPair);
  void SetDumpTime(bool arg) {
    dumpTime = arg;
  }

 protected:
  void CallbackThreadMainStart() override;

 private:
  std::list<std::unique_ptr<MethodProcessTask<T, V>>> tasks;
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_BC_INPUT_BC_PARSER_H
