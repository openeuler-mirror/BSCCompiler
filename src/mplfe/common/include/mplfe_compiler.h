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
#ifndef MPLFE_INCLUDE_COMMON_MPLFE_COMPILER_NEW_H
#define MPLFE_INCLUDE_COMMON_MPLFE_COMPILER_NEW_H
#include <memory>
#include <list>
#include "fe_macros.h"
#include "mplfe_compiler_component.h"
#include "mpl_logging.h"
#include "mplfe_options.h"
#include "jbc_compiler_component.h"
#include "fe_options.h"
#include "bc_compiler_component.h"
#include "ark_annotation_processor.h"
#include "dex_reader.h"
#include "fe_errno.h"
#include "mpl_timer.h"
#include "mplfe_env.h"
#include "fe_manager.h"
#include "fe_type_hierarchy.h"

namespace maple {
class MPLFECompiler {
 public:
  explicit MPLFECompiler(MIRModule &argModule);
  ~MPLFECompiler();
  // common process
  void Run();
  void Init();
  void Release();
  void CheckInput();
  void SetupOutputPathAndName();
  bool LoadMplt();
  void ExportMpltFile();
  void ExportMplFile();

  // component process
  void RegisterCompilerComponent(std::unique_ptr<MPLFECompilerComponent> comp);
  void ParseInputs();
  void LoadOnDemandTypes();
  void PreProcessDecls();
  void ProcessDecls();
  void ProcessPragmas();
  void PreProcessWithFunctions();
  void ProcessFunctions();

 private:
  inline void InsertImportInMpl(const std::list<std::string> &mplt) const;
  void FindMinCompileFailedFEFunctions();
  MIRModule &module;
  MemPool *mp;
  MapleAllocator allocator;
  std::string firstInputName;
  std::string outputPath;
  std::string outputName;
  std::list<std::unique_ptr<MPLFECompilerComponent>> components;
  std::set<FEFunction*> compileFailedFEFunctions;
};
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_MPLFE_COMPILER_NEW_H
