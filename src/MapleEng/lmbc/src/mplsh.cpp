/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#include <gnu/lib-names.h>
#include "massert.h"
#include "lmbc_eng.h"
#include "eng_shim.h"

namespace maple {

void *LmbcMod::FindExtFunc(PUIdx puidx) {
  void* fp = extFuncMap[puidx];
  if (fp) {
    return fp;
  }
  std::string fname = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx)->GetName();
  for (auto it : libHandles) {
    fp = dlsym(it, fname.c_str());
    if (fp) {
      break;
    }
  }
  MASSERT(fp, "dlsym symbol not found: %s", fname.c_str());
  extFuncMap[puidx] = fp;
  return(fp);
}

void *LmbcMod::FindExtSym(StIdx stidx) {
  void* var = extSymMap[stidx.FullIdx()];
  if (var) {
    return var;
  }
  MIRSymbol* sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(stidx.Idx());
  if (sym) {
    for (auto it : libHandles) {
      var = dlsym(it, sym->GetName().c_str());
      if (var) {
        break;
      }
    }
    MASSERT(var, "dlsym ExtSym not found: %s", sym->GetName().c_str());
    extSymMap[stidx.FullIdx()] = var;
  }
  MASSERT(sym, "Unable to find symbol");
  return(var);
}

maple::MIRModule*
LmbcMod::Import(std::string path) {
  maple::MIRModule* mod = new maple::MIRModule(path.c_str());
  mod->SetSrcLang(kSrcLangC);
  std::string::size_type lastdot = mod->GetFileName().find_last_of(".");
  bool islmbc = lastdot != std::string::npos && mod->GetFileName().compare(lastdot, 5, ".lmbc\0") == 0;
  if (!islmbc) {
    ERR(kLncErr, "Input must be .lmbc file: %s", path.c_str());
    delete mod;
    return nullptr;
  }

  BinaryMplImport binMplt(*mod);
  binMplt.SetImported(false);
  std::string modid = mod->GetFileName();
  if (!binMplt.Import(modid, true)) {
    ERR(kLncErr, "mplsh-lmbc: cannot open .lmbc file: %s", modid.c_str());
    delete mod;
    return nullptr;
  }
  return mod;
}

// C runtime libs to preload
// - add to list as needed or change to read list dynamically at runtime 
std::vector<std::string> preLoadLibs = {
  LIBC_SO,
  LIBM_SO
};

void LmbcMod::LoadDefLibs() {
   for (auto it : preLoadLibs) {
    void *handle = dlopen(it.c_str(), RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
    MASSERT(handle, "dlopen %s failed", it.c_str());
    libHandles.push_back(handle);
  }
}

LmbcMod::LmbcMod(char* path) : lmbcPath(path) {
  LoadDefLibs();
  mirMod = Import(path);
  // In Lmbc GlobalMemSize is the mem segment size for un-init
  // PU static variables, and is referenced through %%GP register. 
  unInitPUStaticsSize = mirMod->GetGlobalMemSize();
}

int
RunLmbc(int argc, char** argv) {
  int rc = 1;
  const int skipArgsNum = 1;
  LmbcMod* mod = new LmbcMod(argv[skipArgsNum]);
  ASSERT(mod, "Create Lmbc module failed");
  ASSERT(mod->mirMod, "Import Lmbc module failed");
  mod->InitModule();
  if (mod->mainFn) {
    rc = __engineShim(mod->mainFn, argc-skipArgsNum, argv+skipArgsNum);
  }
  return rc;
}

} // namespace maple

int
main(int argc, char **argv) {
  if (argc == 1) {
    std::string path(argv[0]);
    (void)MIR_PRINTF("usage: %s <file>.lmbc\n", path.substr(path.find_last_of("/\\") + 1).c_str());
    exit(1);
  }
  return maple::RunLmbc(argc, argv);
}
