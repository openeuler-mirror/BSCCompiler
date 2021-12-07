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
#ifndef HIR2MPL_INCLUDE_COMMON_HIR2MPL_COMPILER_NEW_H
#define HIR2MPL_INCLUDE_COMMON_HIR2MPL_COMPILER_NEW_H
#include <memory>
#include <list>
#include "fe_macros.h"
#include "hir2mpl_compiler_component.h"
#include "mpl_logging.h"
#include "hir2mpl_options.h"
#include "jbc_compiler_component.h"
#include "fe_options.h"
#include "bc_compiler_component.h"
#include "ark_annotation_processor.h"
#include "dex_reader.h"
#include "ast_compiler_component.h"
#include "ast_parser.h"
#ifdef ENABLE_MAST
#include "maple_ast_parser.h"
#endif
#include "mpl_timer.h"
#include "hir2mpl_env.h"
#include "fe_manager.h"
#include "fe_type_hierarchy.h"

namespace maple {
class HIR2MPLCompiler {
 public:
  explicit HIR2MPLCompiler(MIRModule &argModule);
  ~HIR2MPLCompiler();
  // common process
  int Run();
  void Init();
  void Release();
  void CheckInput();
  void SetupOutputPathAndName();
  bool LoadMplt();
  void ExportMpltFile();
  void ExportMplFile();

  // component process
  void RegisterCompilerComponent(std::unique_ptr<HIR2MPLCompilerComponent> comp);
  void ParseInputs();
  void LoadOnDemandTypes();
  void PreProcessDecls();
  void ProcessDecls();
  void ProcessPragmas();
  void ProcessFunctions();

 private:
  void RegisterCompilerComponent();
  inline void InsertImportInMpl(const std::list<std::string> &mplt) const;
  void FindMinCompileFailedFEFunctions();
  MIRModule &module;
  MIRSrcLang srcLang;
  MemPool *mp;
  MapleAllocator allocator;
  std::string firstInputName;
  std::string outputPath;
  std::string outputName;
  std::string outputInlineName;
  std::list<std::unique_ptr<HIR2MPLCompilerComponent>> components;
  std::set<FEFunction*> compileFailedFEFunctions;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_HIR2MPL_COMPILER_NEW_H
