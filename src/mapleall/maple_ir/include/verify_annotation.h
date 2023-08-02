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
#ifndef MAPLEIR_VERIFY_ANNOTATION_H
#define MAPLEIR_VERIFY_ANNOTATION_H
#include "mir_module.h"
#include "mir_type.h"
#include "verify_pragma_info.h"

namespace maple {
void AddVerfAnnoThrowVerifyError(MIRModule &md, const ThrowVerifyErrorPragma &info, MIRStructType &clsType);
void AddVerfAnnoAssignableCheck(MIRModule &md,
                                std::vector<const AssignableCheckPragma*> &info,
                                MIRStructType &clsType);
void AddVerfAnnoExtendFinalCheck(MIRModule &md, MIRStructType &clsType);
void AddVerfAnnoOverrideFinalCheck(MIRModule &md, MIRStructType &clsType);
} // namespace maple
#endif // MAPLEALL_VERIFY_ANNOTATION_H