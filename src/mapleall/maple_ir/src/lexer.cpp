/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "lexer.h"
#include <cmath>
#include <cstdlib>
#include "mpl_logging.h"
#include "debug_info.h"
#include "mir_module.h"
#include "securec.h"
#include "utils.h"
#include "int128_util.h"

namespace maple {
int32 HexCharToDigit(char c) {
  int32 ret = utils::ToDigit<16, int32>(c);
  return (ret != INT32_MAX ? ret : 0);
}

static uint8 Char2num(char c) {
  uint8 ret = utils::ToDigit<16>(c);
  ASSERT(ret != UINT8_MAX, "not a hex value");
  return ret;
}

// Read (next) line from the MIR (text) file, and return the read
// number of chars.
// if the line is empty (nothing but a newline), returns 0.
// if EOF, return -1.
// The trailing new-line character has been removed.
int MIRLexer::ReadALine() {
  if (airFile == nullptr) {
    line = "";
    return -1;
  }

  curIdx = 0;
  if (!std::getline(*airFile, line)) {  // EOF
    line = "";
    airFile = nullptr;
    currentLineSize = 0;
    return -1;
  }

  RemoveReturnInline(line);
  currentLineSize = line.length();
  return currentLineSize;
}

MIRLexer::MIRLexer(DebugInfo *debugInfo,  MapleAllocator &alloc)
    : dbgInfo(debugInfo),
      seenComments(alloc.Adapter()),
      keywordMap(alloc.Adapter()) {
  // initialize keywordMap
  keywordMap.clear();
#define KEYWORD(STR)            \
  {                             \
    std::string str;            \
    str = #STR;                 \
    keywordMap[str] = TK_##STR; \
  }
#include "keywords.def"
#undef KEYWORD
}

void MIRLexer::PrepareForFile(const std::string &filename) {
  // open MIR file
  airFileInternal.open(filename);
  CHECK_FATAL(airFileInternal.is_open(), "cannot open MIR file %s\n", &filename);

  airFile = &airFileInternal;
  // try to read the first line
  if (ReadALine() < 0) {
    lineNum = 0;
  } else {
    lineNum = 1;
  }
  UpdateDbgMsg(lineNum);
  kind = TK_invalid;
}

void MIRLexer::UpdateDbgMsg(uint32 dbgLineNum) {
  if (dbgInfo) {
    dbgInfo->UpdateMsg(dbgLineNum, line.c_str());
  }
}

void MIRLexer::PrepareForString(const std::string &src) {
  line = src;
  RemoveReturnInline(line);
  currentLineSize = line.size();
  curIdx = 0;
  NextToken();
}

void MIRLexer::GenName() {
  uint32 startIdx = curIdx;
  char c = GetNextCurrentCharWithUpperCheck();
  char cp = GetCharAt(curIdx - 1);
  if (c == '@' && (cp == 'h' || cp == 'f')) {
    // special pattern for exception handling labels: catch or finally
    c = GetNextCurrentCharWithUpperCheck();
  }
  while (utils::IsAlnum(c) || c < 0 || c == '_' || c == '$' || c == ';' ||
         c == '/' || c == '|' || c == '.' || c == '?' ||
         c == '@') {
    c = GetNextCurrentCharWithUpperCheck();
  }
  name = line.substr(startIdx, curIdx - startIdx);
}

// get the constant value
TokenKind MIRLexer::GetConstVal() {
  bool negative = false;
  int valStart = static_cast<int>(curIdx);
  char c = GetCharAtWithUpperCheck(curIdx);
  if (c == '-') {
    c = GetNextCurrentCharWithUpperCheck();
    TokenKind tk = GetSpecialFloatConst();
    if (tk != TK_invalid) {
      return tk;
    }
    negative = true;
  }
  const uint32 lenHexPrefix = 2;
  if (line.compare(curIdx, lenHexPrefix + 1, "0xL") == 0) {
    curIdx += lenHexPrefix + 1;
    return GetLongHexConst(valStart, negative);
  }

  if (line.compare(curIdx, lenHexPrefix, "0x") == 0) {
    curIdx += lenHexPrefix;
    if (line.compare(curIdx, 1, "L") == 0) {
      curIdx += lenHexPrefix;
    }
    return GetHexConst(valStart, negative);
  }
  uint32 startIdx = curIdx;
  while (isdigit(c)) {
    c = GetNextCurrentCharWithUpperCheck();
  }
  char cs = GetCharAtWithUpperCheck(startIdx);
  if (!isdigit(cs) && c != '.') {
    return TK_invalid;
  }
  if (c != '.' && c != 'f' && c != 'F' && c != 'e' && c != 'E') {
    curIdx = startIdx;
    return GetIntConst(valStart, negative);
  }
  return GetFloatConst(valStart, startIdx, negative);
}

TokenKind MIRLexer::GetSpecialFloatConst() {
  constexpr uint32 lenSpecFloat = 4;
  constexpr uint32 lenSpecDouble = 3;
  if (line.compare(curIdx, lenSpecFloat, "inff") == 0 &&
      !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecFloat))) {
    curIdx += lenSpecFloat;
    theFloatVal = -INFINITY;
    return TK_floatconst;
  }
  if (line.compare(curIdx, lenSpecDouble, "inf") == 0 &&
      !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecDouble))) {
    curIdx += lenSpecDouble;
    theDoubleVal = -INFINITY;
    return TK_doubleconst;
  }
  if (line.compare(curIdx, lenSpecFloat, "nanf") == 0 &&
      !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecFloat))) {
    curIdx += lenSpecFloat;
    theFloatVal = -NAN;
    return TK_floatconst;
  }
  if (line.compare(curIdx, lenSpecDouble, "nan") == 0 &&
      !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecDouble))) {
    curIdx += lenSpecDouble;
    theDoubleVal = -NAN;
    return TK_doubleconst;
  }
  return TK_invalid;
}

TokenKind MIRLexer::GetHexConst(uint32 valStart, bool negative) {
  char c = GetCharAtWithUpperCheck(curIdx);
  if (!isxdigit(c)) {
    name = line.substr(valStart, curIdx - valStart);
    return TK_invalid;
  }
  IntVal tmp(static_cast<uint64>(HexCharToDigit(c)), kInt128BitSize, negative);
  c = GetNextCurrentCharWithUpperCheck();
  while (isxdigit(c)) {
    tmp = (tmp << 4) + static_cast<uint32>(HexCharToDigit(c));
    c = GetNextCurrentCharWithUpperCheck();
  }
  if (negative) {
    tmp = -tmp;
  }
  theIntVal = tmp.Trunc(PTY_i64).GetExtValue();
  theFloatVal = static_cast<float>(theIntVal);
  theDoubleVal = static_cast<double>(theIntVal);
  if (negative && theIntVal == 0) {
    theFloatVal = -theFloatVal;
    theDoubleVal = -theDoubleVal;
  }
  theInt128Val.Assign(tmp);
  name = line.substr(valStart, curIdx - valStart);
  return TK_intconst;
}

TokenKind MIRLexer::GetLongHexConst(uint32 valStart, bool negative) {
  char c = GetCharAtWithUpperCheck(curIdx);
  if (!isxdigit(c)) {
    name = line.substr(valStart, curIdx - valStart);
    return TK_invalid;
  }
  unsigned __int128 tmp = 0;
  uint32 buf = 0;
  while (isxdigit(c)) {
    buf = static_cast<uint32>(HexCharToDigit(c));
    tmp = (tmp << 4) + buf;
    c = GetNextCurrentCharWithUpperCheck();
  }
  theLongDoubleVal[1] = static_cast<uint64>(tmp);
  theLongDoubleVal[0] = static_cast<uint64>(tmp >> 64);
  theIntVal = static_cast<int64>(static_cast<uint64>(theLongDoubleVal[1]));
  if (negative) {
    theIntVal = -theIntVal;
  }
  theFloatVal = static_cast<float>(theIntVal);
  theDoubleVal = static_cast<double>(theIntVal);
  if (negative && theIntVal == 0) {
    theFloatVal = -theFloatVal;
    theDoubleVal = -theDoubleVal;
  }
  name = line.substr(valStart, curIdx - valStart);
  return TK_intconst;
}

TokenKind MIRLexer::GetIntConst(uint32 valStart, bool negative) {
  char c = GetCharAtWithUpperCheck(curIdx);
  if (!isxdigit(c)) {
    name = line.substr(valStart, curIdx - valStart);
    return TK_invalid;
  }
  uint64 radix = HexCharToDigit(c) == 0 ? 8 : 10;
  IntVal tmp(static_cast<uint64>(0), kInt128BitSize, negative);
  while (isdigit(c)) {
    tmp = (tmp * radix) + static_cast<uint64>(HexCharToDigit(c));
    c = GetNextCurrentCharWithUpperCheck();
  }

  if (c == 'u' || c == 'U') {  // skip 'u' or 'U'
    c = GetNextCurrentCharWithUpperCheck();
    if (c == 'l' || c == 'L') {
      c = GetNextCurrentCharWithUpperCheck();
    }
  }

  if (c == 'l' || c == 'L') {
    c = GetNextCurrentCharWithUpperCheck();
    if (c == 'l' || c == 'L' || c == 'u' || c == 'U') {
      ++curIdx;
    }
  }

  name = line.substr(valStart, curIdx - valStart);

  if (negative) {
    tmp = -tmp;
  }

  theInt128Val.Assign(tmp);
  theIntVal = tmp.Trunc(PTY_u64).GetExtValue();
  if (negative) {
    theFloatVal = static_cast<float>(static_cast<int64>(theIntVal));
    theDoubleVal = static_cast<double>(static_cast<int64>(theIntVal));

    if (theIntVal == 0) {
      theFloatVal = -theFloatVal;
      theDoubleVal = -theDoubleVal;
    }
  } else {
    theFloatVal = static_cast<float>(theIntVal);
    theDoubleVal = static_cast<double>(theIntVal);
  }

  return TK_intconst;
}

TokenKind MIRLexer::GetFloatConst(uint32 valStart, uint32 startIdx, bool negative) {
  char c = GetCharAtWithUpperCheck(curIdx);
  if (c == '.') {
    c = GetNextCurrentCharWithUpperCheck();
  }
  while (isdigit(c)) {
    c = GetNextCurrentCharWithUpperCheck();
  }
  bool doublePrec = true;
  if (c == 'e' || c == 'E') {
    c = GetNextCurrentCharWithUpperCheck();
    if (!isdigit(c) && c != '-' && c != '+') {
      name = line.substr(valStart, curIdx - valStart);
      return TK_invalid;
    }
    if (c == '-' || c == '+') {
      c = GetNextCurrentCharWithUpperCheck();
    }
    while (isdigit(c)) {
      c = GetNextCurrentCharWithUpperCheck();
    }
  }
  if (c == 'f' || c == 'F') {
    doublePrec = false;
    c = GetNextCurrentCharWithUpperCheck();
  }
  if (c == 'l' || c == 'L') {
    c = GetNextCurrentCharWithUpperCheck();
  }

  std::string floatStr = line.substr(startIdx, curIdx - startIdx);
  // get the float constant value
  if (!doublePrec) {
    int eNum = sscanf_s(floatStr.c_str(), "%e", &theFloatVal);
    CHECK_FATAL(eNum == 1, "sscanf_s failed");

    if (negative) {
      theFloatVal = -theFloatVal;
    }
    theIntVal = static_cast<int64>(theFloatVal);
    theDoubleVal = static_cast<double>(theFloatVal);
    if (negative && fabs(theFloatVal) <= 1e-6) {
      theDoubleVal = -theDoubleVal;
    }
    name = line.substr(valStart, curIdx - valStart);
    return TK_floatconst;
  } else {
    int eNum = sscanf_s(floatStr.c_str(), "%le", &theDoubleVal);
    CHECK_FATAL(eNum == 1, "sscanf_s failed");

    if (negative) {
      theDoubleVal = -theDoubleVal;
    }
    theIntVal = static_cast<int64>(theDoubleVal);
    theFloatVal = static_cast<float>(theDoubleVal);
    if (negative && fabs(theDoubleVal) <= 1e-15) {
      theFloatVal = -theFloatVal;
    }
    name = line.substr(valStart, curIdx - valStart);
    return TK_doubleconst;
  }
}

TokenKind MIRLexer::GetTokenWithPrefixDollar() {
  // token with prefix '$'
  char c = GetCharAtWithUpperCheck(curIdx);
  if (utils::IsAlpha(c) || c == '_' || c == '$') {
    GenName();
    return TK_gname;
  } else {
    // for error reporting.
    const uint32 printLength = 2;
    name = line.substr(curIdx - 1, printLength);
    return TK_invalid;
  }
}

TokenKind MIRLexer::GetTokenWithPrefixPercent() {
  // token with prefix '%'
  char c = GetCharAtWithUpperCheck(curIdx);
  if (isdigit(c)) {
    int valStart = static_cast<int>(curIdx) - 1;
    theIntVal = static_cast<int64>(HexCharToDigit(c));
    c = GetNextCurrentCharWithUpperCheck();
    while (isdigit(c)) {
      theIntVal = (theIntVal * 10) + static_cast<int64>(HexCharToDigit(c));
      ASSERT(theIntVal >= 0, "int value overflow");
      c = GetNextCurrentCharWithUpperCheck();
    }
    name = line.substr(valStart, curIdx - valStart);
    return TK_preg;
  }
  if (utils::IsAlpha(c) || c == '_' || c == '$') {
    GenName();
    return TK_lname;
  }
  if (c == '%' && utils::IsAlpha(GetCharAtWithUpperCheck(curIdx + 1))) {
    ++curIdx;
    GenName();
    return TK_specialreg;
  }
  return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixAmpersand() {
  // token with prefix '&'
  char c = GetCurrentCharWithUpperCheck();
  if (utils::IsAlpha(c) || c == '_') {
    GenName();
    return TK_fname;
  }
  // for error reporting.
  constexpr uint32 printLength = 2;
  name = line.substr(curIdx - 1, printLength);
  return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixAtOrCircumflex(char prefix) {
  // token with prefix '@' or `^`
  char c = GetCurrentCharWithUpperCheck();
  if (utils::IsAlnum(c) || c < 0 || c == '_' || c == '@' || c == '$' || c == '|') {
    GenName();
    if (prefix == '@') {
      return TK_label;
    }
    return TK_prntfield;
  }
  return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixExclamation() {
  // token with prefix '!'
  char c = GetCurrentCharWithUpperCheck();
  if (utils::IsAlpha(c)) {
    GenName();
    return TK_typeparam;
  }
  if (utils::IsDigit(c)) {
    (void)GetConstVal();
    return TK_exclamation;
  }
  // for error reporting.
  const uint32 printLength = 2;
  name = line.substr(curIdx - 1, printLength);
  return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixQuotation() {
  if (GetCharAtWithUpperCheck(curIdx + 1) == '\'') {
    theIntVal = static_cast<int64_t>(GetCharAtWithUpperCheck(curIdx));
    curIdx += 2;
    return TK_intconst;
  }
  return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixDoubleQuotation() {
  uint32 startIdx = curIdx;
  uint32 shift = 0;
  // for \", skip the \ to leave " only internally
  // and also for the pair of chars \ and n become '\n' etc.
  char c = GetCurrentCharWithUpperCheck();
  while ((c != 0) && (c != '\"' || GetCharAtWithLowerCheck(curIdx - 1) == '\\')) {
    if (GetCharAtWithLowerCheck(curIdx - 1) == '\\') {
      shift++;
      switch (c) {
        case '"':
          line[curIdx - shift] = c;
          break;
        case '\\':
          line[curIdx - shift] = c;
          // avoid 3rd \ in \\\ being treated as an escaped one
          line[curIdx] = 0;
          break;
        case 'a':
          line[curIdx - shift] = '\a';
          break;
        case 'b':
          line[curIdx - shift] = '\b';
          break;
        case 't':
          line[curIdx - shift] = '\t';
          break;
        case 'n':
          line[curIdx - shift] = '\n';
          break;
        case 'v':
          line[curIdx - shift] = '\v';
          break;
        case 'f':
          line[curIdx - shift] = '\f';
          break;
        case 'r':
          line[curIdx - shift] = '\r';
          break;
        // support hex value \xNN
        case 'x': {
          const uint32 hexShift = 4;
          const uint32 hexLength = 2;
          uint8 c1 = Char2num(GetCharAtWithLowerCheck(curIdx + 1));
          uint8 c2 = Char2num(GetCharAtWithLowerCheck(curIdx + 2));
          uint8 cNew = static_cast<uint8_t>(c1 << hexShift) + c2;
          line[curIdx - shift] = static_cast<char>(cNew);
          curIdx += hexLength;
          shift += hexLength;
          break;
        }
        // support oct value \NNN
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
          const uint32 octShift1 = 3;
          const uint32 octShift2 = 6;
          const uint32 octLength = 3;
          ASSERT(curIdx + octLength < line.size(), "index out of range");
          uint32 cNew =
              (static_cast<uint8>(static_cast<uint8>(GetCharAtWithLowerCheck(curIdx + 1)) - static_cast<uint8>('0')) <<
               octShift2) +
              (static_cast<uint8>(static_cast<uint8>(GetCharAtWithLowerCheck(curIdx + 2)) - static_cast<uint8>('0')) <<
               octShift1) +
              static_cast<uint8>(static_cast<uint8>(GetCharAtWithLowerCheck(curIdx + 3)) - static_cast<uint8>('0'));
          line[curIdx - shift] = cNew;
          curIdx += octLength;
          shift += octLength;
          break;
        }
        default:
          line[curIdx - shift] = '\\';
          --shift;
          line[curIdx - shift] = c;
          break;
      }
    } else if (shift != 0) {
      line[curIdx - shift] = c;
    }
    c = GetNextCurrentCharWithUpperCheck();
  }
  if (c != '\"') {
    return TK_invalid;
  }
  // for empty string
  if (startIdx == curIdx) {
    name = "";
  } else {
    name = line.substr(startIdx, curIdx - startIdx - shift);
  }
  ++curIdx;
  return TK_string;
}

TokenKind MIRLexer::GetTokenSpecial() {
  --curIdx;
  char c = GetCharAtWithLowerCheck(curIdx);
  if (utils::IsAlpha(c) || c < 0 || c == '_') {
    GenName();
    TokenKind tk = keywordMap[name];
    switch (tk) {
      case TK_nanf:
        theFloatVal = NAN;
        return TK_floatconst;
      case TK_nan:
        theDoubleVal = NAN;
        return TK_doubleconst;
      case TK_inff:
        theFloatVal = INFINITY;
        return TK_floatconst;
      case TK_inf:
        theDoubleVal = INFINITY;
        return TK_doubleconst;
      default:
        return tk;
    }
  }
  MIR_ERROR("error in input file\n");
  return TK_eof;
}

TokenKind MIRLexer::LexToken() {
  // skip spaces
  char c = GetCurrentCharWithUpperCheck();
  while (c == ' ' || c == '\t') {
    c = GetNextCurrentCharWithUpperCheck();
  }
  // check end of line
  while (c == 0 || c == '#') {
    if (c == '#') {  // process comment contents
      seenComments.push_back(line.substr(curIdx + 1, currentLineSize - curIdx - 1));
    }
    if (ReadALine() < 0) {
      return TK_eof;
    }
    ++lineNum;  // a new line read.
    UpdateDbgMsg(lineNum);
    // skip spaces
    c = GetCurrentCharWithUpperCheck();
    while (c == ' ' || c == '\t') {
      c = GetNextCurrentCharWithUpperCheck();
    }
  }
  char curChar = c;
  ++curIdx;
  switch (curChar) {
    case '\n':
      return TK_newline;
    case '(':
      return TK_lparen;
    case ')':
      return TK_rparen;
    case '{':
      return TK_lbrace;
    case '}':
      return TK_rbrace;
    case '[':
      return TK_lbrack;
    case ']':
      return TK_rbrack;
    case '<':
      return TK_langle;
    case '>':
      return TK_rangle;
    case '=':
      return TK_eqsign;
    case ',':
      return TK_coma;
    case ':':
      return TK_colon;
    case '*':
      return TK_asterisk;
    case '.':
      if (GetCharAtWithUpperCheck(curIdx) == '.') {
        const uint32 lenDotdot = 2;
        curIdx += lenDotdot;
        return TK_dotdotdot;
      }
    // fall thru for .9100 == 0.9100
    [[clang::fallthrough]];
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      --curIdx;
      return GetConstVal();
    case '$':
      return GetTokenWithPrefixDollar();
    case '%':
      return GetTokenWithPrefixPercent();
    case '&':
      return GetTokenWithPrefixAmpersand();
    case '@':
    case '^':
      return GetTokenWithPrefixAtOrCircumflex(curChar);
    case '!':
      if (kind == TK_pragma) {
        return TK_exclamation;
      }
      return GetTokenWithPrefixExclamation();
    case '\'':
      return GetTokenWithPrefixQuotation();
    case '\"':
      return GetTokenWithPrefixDoubleQuotation();
    default:
      return GetTokenSpecial();
  }
}

TokenKind MIRLexer::NextToken() {
  kind = LexToken();
  return kind;
}

std::string MIRLexer::GetTokenString() const {
  std::string temp;
  switch (kind) {
    case TK_gname: {
      temp = "$";
      temp.append(name);
      return temp;
    }
    case TK_lname:
    case TK_preg: {
      temp = "%";
      temp.append(name);
      return temp;
    }
    case TK_specialreg: {
      temp = "%%";
      temp.append(name);
      return temp;
    }
    case TK_label: {
      temp = "@";
      temp.append(name);
      return temp;
    }
    case TK_prntfield: {
      temp = "^";
      temp.append(name);
      return temp;
    }
    case TK_intconst: {
      temp = std::to_string(theIntVal);
      return temp;
    }
    case TK_floatconst: {
      temp = std::to_string(theFloatVal);
      return temp;
    }
    case TK_doubleconst: {
      temp = std::to_string(theDoubleVal);
      return temp;
    }
    // misc.
    case TK_newline: {
      temp = "\\n";
      return temp;
    }
    case TK_lparen: {
      temp = "(";
      return temp;
    }
    case TK_rparen: {
      temp = ")";
      return temp;
    }
    case TK_lbrace: {
      temp = "{";
      return temp;
    }
    case TK_rbrace: {
      temp = "}";
      return temp;
    }
    case TK_lbrack: {
      temp = "[";
      return temp;
    }
    case TK_rbrack: {
      temp = "]";
      return temp;
    }
    case TK_langle: {
      temp = "<";
      return temp;
    }
    case TK_rangle: {
      temp = ">";
      return temp;
    }
    case TK_eqsign: {
      temp = "=";
      return temp;
    }
    case TK_coma: {
      temp = ",";
      return temp;
    }
    case TK_dotdotdot: {
      temp = "...";
      return temp;
    }
    case TK_colon: {
      temp = ":";
      return temp;
    }
    case TK_asterisk: {
      temp = "*";
      return temp;
    }
    case TK_string: {
      temp = "\"";
      temp.append(name);
      temp.append("\"");
      return temp;
    }
    default:
      temp = "invalid token";
      return temp;
  }
}
}  // namespace maple
