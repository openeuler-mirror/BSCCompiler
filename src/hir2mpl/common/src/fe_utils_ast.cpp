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
#include "fe_utils_ast.h"
#include <map>
#include "mpl_logging.h"

namespace maple {
PrimType FEUtilAST::GetTypeFromASTTypeName(const std::string &typeName) {
  const static std::map<std::string, PrimType> mapASTTypeNameToType = {
      {"bool", PTY_u1},
      {"uint8", PTY_u8},
      {"uint16", PTY_u16},
      {"uint32", PTY_u32},
      {"uint64", PTY_u64},
      {"int8", PTY_i8},
      {"int16", PTY_i16},
      {"int32", PTY_i32},
      {"int64", PTY_i64},
      {"float", PTY_f32},
      {"double", PTY_f64},
      {"void", PTY_void}
  };
  auto it = mapASTTypeNameToType.find(typeName);
  CHECK_FATAL(it != mapASTTypeNameToType.end(), "Can not find typeName %s", typeName.c_str());
  return it->second;
}

const std::string FEUtilAST::Type2Label(PrimType primType) {
  switch (primType) {
    case PTY_u1:
      return "B";
    case PTY_i8:
      return "A";
    case PTY_u8:
      return "C";
    case PTY_i16:
      return "S";
    case PTY_u16:
      return "T";
    case PTY_i32:
      return "I";
    case PTY_u32:
      return "M";
    case PTY_i64:
      return "O";
    case PTY_u64:
      return "Q";
    case PTY_f32:
      return "F";
    case PTY_f64:
      return "D";
    case PTY_void:
      return "V";
    default:
      return "R";
  }
}
}  // namespace maple
