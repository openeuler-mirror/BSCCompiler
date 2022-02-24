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

#include "x64_emitter.h"
namespace maplebe {
void X64Emitter::EmitRefToMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {}
void X64Emitter::EmitRefToMethodInfo(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {}
void X64Emitter::EmitMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {}
void X64Emitter::EmitFastLSDA(FuncEmitInfo &funcEmitInfo) {}
void X64Emitter::EmitFullLSDA(FuncEmitInfo &funcEmitInfo) {}
void X64Emitter::EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) {}
void X64Emitter::EmitJavaInsnAddr(FuncEmitInfo &funcEmitInfo) {}
void X64Emitter::Run(FuncEmitInfo &funcEmitInfo) {}

bool CgEmission::PhaseRun(maplebe::CGFunc &f) {
  return false;
}
}  /* namespace maplebe */
