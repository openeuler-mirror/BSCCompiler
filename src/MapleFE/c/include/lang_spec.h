/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEFE_LANG_SPEC_H
#define MAPLEFE_LANG_SPEC_H
#include "stringutil.h"
#include "token.h"
#include "lexer.h"

namespace maplefe {

class StringToValueImpl : public StringToValue {
public:
  float  StringToFloat(std::string &s);
  double StringToDouble(std::string &s);
  bool   StringToBool(std::string &s);
  Char   StringToChar(std::string &s);
  bool   StringIsNull(std::string &s);
};

extern LitData ProcessLiteral(LitId type, const char *str);

class CLexer : public Lexer {
};

}
#endif //MAPLEFE_LANG_SPEC_H
