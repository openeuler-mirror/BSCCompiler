/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "x64_MPISel.h"

namespace maplebe {
CGMemOperand &X64MPIsel::GetSymbolFromMemory(const MIRSymbol &symbol) {
  MIRStorageClass storageClass = symbol.GetStorageClass();
  CGMemOperand *result = nullptr;
  if ((storageClass == kScAuto) || (storageClass == kScFormal)) {
    uint32 opndSz = GetPrimTypeSize(symbol.GetType()->GetPrimType()) * kBitsPerByte;
    result = &GetCurFunc()->GetOpndBuilder()->CreateMem(opndSz);
    /* memoperand */
  } else {
    CHECK_FATAL(false, "NIY");
  }
  CHECK_FATAL(result != nullptr, "NIY");
  return *result;
}
}
