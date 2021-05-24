/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*
*  http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/
#include <iostream>
#include <cmath>
#include "massert.h"
#include "lexer.h"
#include "token.h"
#include "ruletable_util.h"
#include "rule_summary.h"
#include "massert.h"
#include <climits>
#include <cstdlib>

namespace maplefe {

#define MAX_LINE_SIZE 4096

/* Read (next) line from the input file, and return the readed
   number of chars.
   if the line is empty (nothing but a newline), returns 0.
   if EOF, return -1.
   The trailing new-line character has been removed.
 */
int Lexer::ReadALine() {
  if (!srcfile) {
    line[0] = '\0';
    return -1;
  }

  current_line_size = getline(&line, &linebuf_size, srcfile);
  if (current_line_size <= 0) {  // EOF
    fclose(srcfile);
    line[0] = '\0';
    endoffile = true;
  } else {
    // There could be \n\r or \r\n
    // Handle the last escape
    if ((line[current_line_size - 1] == '\n') ||
        (line[current_line_size - 1] == '\r')) {
      line[current_line_size - 1] = '\0';
      current_line_size--;
    }
    // Handle the second last escape
    if ((line[current_line_size - 1] == '\n') ||
        (line[current_line_size - 1] == '\r')) {
      line[current_line_size - 1] = '\0';
      current_line_size--;
    }
  }

  curidx = 0;
  return current_line_size;
}

Lexer::Lexer()
  : thename(""),
    theintval(0),
    thefloatval(0.0f),
    thedoubleval(0),
    verboseLevel(0),
    srcfile(nullptr),
    line(nullptr),
    linebuf_size(0),
    current_line_size(0),
    curidx(0),
    endoffile(false),
    mPredefinedTokenNum(0),
    mTrace(false),
    mLineMode(false),
    _linenum(0) {
      seencomments.clear();
      mCheckSeparator = true;
      mMatchToken = true;
      mLastLiteralId = LT_NullLiteral;
}

void Lexer::PrepareForFile(const std::string filename) {
  // open file
  srcfile = fopen(filename.c_str(), "r");
  if (!srcfile) {
    std::cerr << "cannot open file " << filename << std::endl;
    exit(1);
  }

  // allocate line buffer.
  linebuf_size = (size_t)MAX_LINE_SIZE;
  line = static_cast<char *>(malloc(linebuf_size));  // initial line buffer.
  if (!line) {
    MASSERT("cannot allocate line buffer\n");
  }

  // try to read the first line
  if (ReadALine() < 0) {
    _linenum = 0;
  } else {
    _linenum = 1;
  }
}

void Lexer::PrepareForString(const char *str) {
  current_line_size = strlen(str);
  strncpy(line, str, current_line_size);
  line[current_line_size] = '\0';
  curidx = 0;
  _linenum = 1;
  endoffile = false;
}

///////////////////////////////////////////////////////////////////////////
//                Utilities for finding system tokens
// Remember the order of tokens are operators, separators, and keywords.
///////////////////////////////////////////////////////////////////////////

Token* Lexer::FindOperatorToken(OprId id) {
  Token *token = NULL;
  bool found = false;
  for (unsigned i = 0; i < gOperatorTokensNum; i++) {
    token = &gSystemTokens[i];
    MASSERT(token->mTkType == TT_OP);
    if (token->GetOprId() == id) {
      found = true;
      break;
    }
  }
  MASSERT(found && token);
  return token;
}

Token* Lexer::FindSeparatorToken(SepId id) {
  Token *token = NULL;
  bool found = false;
  for (unsigned i = gOperatorTokensNum; i < gOperatorTokensNum + gSeparatorTokensNum; i++) {
    token = &gSystemTokens[i];
    MASSERT(token->mTkType == TT_SP);
    if (token->GetSepId() == id) {
      found = true;
      break;
    }
  }
  MASSERT(found && token);
  return token;
}

// The caller of this function makes sure 'key' is already in the
// string pool of Lexer.
Token* Lexer::FindKeywordToken(const char *key) {
  Token *token = NULL;
  bool found = false;
  for (unsigned i = gOperatorTokensNum + gSeparatorTokensNum;
       i < gOperatorTokensNum + gSeparatorTokensNum + gKeywordTokensNum;
       i++) {
    token = &gSystemTokens[i];
    MASSERT(token->mTkType == TT_KW);
    if (strlen(key) == strlen(token->GetName()) &&
        !strncmp(key, token->GetName(), strlen(key))) {
      found = true;
      break;
    }
  }
  MASSERT(found && token);
  return token;
}

// CommentToken is the last predefined token
Token* Lexer::FindCommentToken() {
  Token *token = &gSystemTokens[gSystemTokensNum - 1];
  MASSERT((token->mTkType == TT_CM) && "Last system token is not a comment token.");
  return token;
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

// Read a token until end of file.
// If no remaining tokens in current line, we move to the next line.
Token* Lexer::LexToken(void) {
  return LexTokenNoNewLine();
}

// Read a token until end of line.
// Return NULL if no token read.
Token* Lexer::LexTokenNoNewLine(void) {
  bool is_comment = GetComment();
  if (is_comment) {
    Token *t = FindCommentToken();
    if (mTrace)
      t->Dump();
    return t;
  }

  // We try to get system tokens in the order of operator, separtor, and keyword
  // This is the same as token_gen.cpp in autogen. There is a reason behind this.
  // Some languages could have one synatx belonging to both separator and operators.
  // eg., ':' in Java 8, it's both a separator colon and operator select.
  // So the order here must be consistent with autogen where it decides ':' a colon or
  // select in a rule.

  OprId opr = GetOperator();
  if (opr != OPR_NA) {
    Token *t = FindOperatorToken(opr);
    if (mTrace)
      t->Dump();
    return t;
  }

  SepId sep = GetSeparator();
  if (sep != SEP_NA) {
    Token *t = FindSeparatorToken(sep);
    if (mTrace)
      t->Dump();
    return t;
  }

  const char *keyword = GetKeyword();
  if (keyword != NULL) {
    Token *t = FindKeywordToken(keyword);
    if (mTrace)
      t->Dump();
    return t;
  }

  LitData ld = GetLiteral();
  if (ld.mType != LT_NA) {
    Token *t = (Token*)mTokenPool.NewToken(sizeof(Token)); 
    t->SetLiteral(ld);
    if (mTrace)
      t->Dump();
    return t;
  }

  const char *identifier = GetIdentifier();
  if (identifier != NULL) {
    Token *t = (Token*)mTokenPool.NewToken(sizeof(Token));
    t->SetIdentifier(identifier);
    if (mTrace)
      t->Dump();
    return t;
  }

  TempLitData* tldata = GetTempLit();
  if (tldata != NULL) {
    Token *t = (Token*)mTokenPool.NewToken(sizeof(Token)); 
    t->SetTempLit(tldata);
    if (mTrace)
      t->Dump();
    return t;
  }

  return NULL;
}

// NOTE: right now we rely on 'tsc' to assure the input is legal,
//       so I'll make many things easier and will skip many lexical
//       checks. Just make it easy for now.
//       Also, I assume we don't handle multiple line template literal
//       for the time being.
TempLitData* Lexer::GetTempLit() {
  TempLitData *tld = NULL;
  unsigned old_cur_idx = curidx;

  if (line[curidx] == '`') {
    // It's certain that this is a template literal because tsc assures it.
    tld = new TempLitData;

    unsigned start_idx;
    unsigned end_idx;
    start_idx = curidx + 1;
    while(1) {
      // Try string
      end_idx = 0;
      bool s_found = FindNextTLFormat(start_idx, end_idx);
      const char *addr = NULL;
      if (s_found) {
        unsigned len = end_idx - start_idx + 1;
        MASSERT(len > 0 && "found token has 0 data?");
        std::string s(GetLine() + start_idx, len);
        addr = gStringPool.FindString(s);
        start_idx = end_idx + 1;
      }

      // Try pattern
      end_idx = 0;
      const char *addr_ph = NULL;
      bool p_found = FindNextTLPlaceHolder(start_idx, end_idx);
      if (p_found) {
        unsigned p_s = start_idx + 2;
        unsigned len = end_idx - p_s + 1;
        MASSERT(len > 0 && "found token has 0 data?");
        std::string s(GetLine() + p_s, len);
        addr_ph = gStringPool.FindString(s);
        // We need skip the ending '}' of a pattern.
        start_idx = end_idx + 2;
      }

      // If both string and pattern failed to be found
      if (!s_found && !p_found) {
        break;
      } else {
        tld->mStrings.PushBack(addr);
        tld->mStrings.PushBack(addr_ph);
      }
    }

    // It's for sure that this is the ending '`'.
    MASSERT(line[start_idx] == '`');
    curidx = start_idx + 1;
  }

  return tld;
}

// Find the pure string of a template literal.
// Set end_idx as the last char of string.
bool Lexer::FindNextTLFormat(unsigned start_idx, unsigned& end_idx) {
  unsigned working_idx = start_idx;
  while(1) {
    if ((line[working_idx] == '$' && line[working_idx+1] == '{')
        || line[working_idx] == '`' ){
      end_idx = working_idx - 1;
      break;
    }
    working_idx++;
  }

  if (end_idx >= start_idx && end_idx <= 0xFFFFFF)
    return true;
  else
    return false;
}

// Find the pattern string of a template literal.
// Set end_idx as the last char of string.
bool Lexer::FindNextTLPlaceHolder(unsigned start_idx, unsigned& end_idx) {
  unsigned working_idx = start_idx;
  if (line[working_idx] != '$' || line[working_idx+1] != '{')
    return false;

  working_idx = start_idx + 2;
  while(1) {
    if (line[working_idx] == '}') {
      break;
    }
    working_idx++;
  }

  end_idx = working_idx - 1;
  return true;
}

// Returen the separator ID, if it's. Or SEP_NA.
SepId Lexer::GetSeparator() {
  return TraverseSepTable();
}

// Returen the operator ID, if it's. Or OPR_NA.
OprId Lexer::GetOperator() {
  return TraverseOprTable();
}

// keyword string was put into gStringPool by walker.TraverseKeywordTable().
const char* Lexer::GetKeyword() {
  const char *addr = TraverseKeywordTable();
  return addr;
}

// identifier string was put into gStringPool.
// NOTE: Identifier table is always Hard Coded as TblIdentifier.
const char* Lexer::GetIdentifier() {
  unsigned old_pos = GetCuridx();
  bool found = Traverse(&TblIdentifier);
  if (found) {
    unsigned len = GetCuridx() - old_pos;
    MASSERT(len > 0 && "found token has 0 data?");
    std::string s(GetLine() + old_pos, len);
    const char *addr = gStringPool.FindString(s);
    return addr;
  } else {
    SetCuridx(old_pos);
    return NULL;
  }
}

// Literal rules are special, an element of the rules may be a char, or a string, and they
// are not followed by separators. They may be followed by another char or string. So we
// don't check if the following is a separator or not.
LitData Lexer::GetLiteral() {
  mCheckSeparator = false;

  unsigned old_pos = GetCuridx();

  LitData ld;
  ld.mType = LT_NA;
  bool found = Traverse(&TblLiteral);
  if (found) {
    unsigned len = GetCuridx() - old_pos;
    MASSERT(len > 0 && "found token has 0 data?");
    std::string s(GetLine() + old_pos, len);
    if (mTrace)
      std::cout << "text:" << s << std::endl;
    const char *addr = gStringPool.FindString(s);
    // We just support integer token right now. Value is put in LitData.mData.mStr
    ld = ProcessLiteral(mLastLiteralId, addr);
  } else {
    SetCuridx(old_pos);
  }

  mCheckSeparator = true;

  return ld;
}

// For comments we are not going to generate rule tables since the most common comment
// grammar are used widely in almost every language. So we decided to have a common
// implementation here. In case any language has specific un-usual grammar, they can
// have their own implementation.
//
// The two common comments are
//  (1) //
//      This is the end of line
//  (2) /* .. */
//      This is the traditional comments
//
// Return true if a comment is read. The contents are ignore.
bool Lexer::GetComment() {
  if (line[curidx] == '/' && line[curidx+1] == '/') {
    curidx = current_line_size;
    return true;
  }

  // Handle comments in /* */
  // If there is a /* without ending */, the rest of code until the end of the current
  // source file will be treated as comment.
  if (line[curidx] == '/' && line[curidx+1] == '*') {
    curidx += 2; // skip /*
    bool get_ending = false;  // if we get the ending */

    // the while loop stops only at either (1) end of file (2) finding */
    while (1) {
      if (curidx == current_line_size) {
        int len = ReadALine();
        while (len == 0)
          len = ReadALine();
        if (len < 0)
          return true;
        _linenum++;  // a new line read.
      }
      if ((line[curidx] == '*' && line[curidx+1] == '/')) {
        get_ending = true;
        curidx += 2;
        break;
      }
      curidx++;
    }
    return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
//           Traverse The Rule Tables during Lexing
//
// Traverse the rule table by reading tokens through lexer.
// Sub Tables are traversed recursively.
//
// It returns true : if RuleTable is met
//           false : if failed
//
// The curidx will move if we successfully found something. So the string
// of found token can be obtained by the difference of 'new' and 'old' curidx.
//
//  ======================= About Second Try ============================
//
// According to the spec of some languages, e.g. Java. It has productions for Hex
// numerals as below.
//
// rule HexDigitOrUnderscore : ONEOF(HexDigit, '_')
// rule HexDigitsAndUnderscores:HexDigitOrUnderscore + ZEROORMORE(HexDigitOrUnderscore)
// rule HexDigits  : ONEOF(HexDigit,
//             ------->    HexDigit + ZEROORONE(HexDigitsAndUnderscores) + HexDigit)
//
// The problem comes from the line pointed by arrow. Look at the ZEROORONE(...),
// it's easy to see that for a string like "123", ZEROORONE(...) will eat up all the
// characters until the end of string. Which means the third element, or the HexDigit
// at the end will never get a chance to match.
//
// So we come up with a second try. The idea is simple, we want to try a second time.
// [NOTE] we only handle the case where the production has the following patten,
//           ... + ZEROORONE(...) + LASTELEMENT  or
//           ... + ZEROORMOREE(...) + LASTELEMENT
// [NOTE] We handle only one ZEROORMORE or ZEROORONE case.
//
// The algorithm is a loop walking on the line, starting at curidx.
// Suppose it's saved at init_idx.
//   1. We skip ZEROORXXX and try to match LAST_ELEMENT. Suppose the starting index is
//      saved as start_idx
//   2. If success, we continue matching line[init_idx, start_idx] against ZEROORXXX
//   3. if success again, we move start_idx by one, repeat step 1.
// This is just a rough idea. Many border conditions will be checked while walking.
///////////////////////////////////////////////////////////////////////////////////


// The Lexer cursor moves if found target, or restore the original location.
bool Lexer::TraverseTableData(TableData *data) {
  unsigned old_pos = curidx;
  bool found = false;

  switch (data->mType) {

  // The first thinking is I also want to check if the next text after 'curidx' is a separtor
  // or operator.
  //
  // This is the case in parsing a DT_String. However, we have many rules handling DT_Char
  // and they don't expect the following to be a separator. For example, the DecimalNumeral rule
  // specifies its content char by char, so in this case we don't check if the next is a separator.
  case DT_Char: {
    if(*(line + curidx) == data->mData.mChar) {
      curidx += 1;
      found = true;
    }
    break;
  }

  case DT_String: {
    if( !strncmp(line + curidx, data->mData.mString, strlen(data->mData.mString))) {
      // Need to make sure the following text is a separator
      curidx += strlen(data->mData.mString);
      if (mCheckSeparator && (TraverseSepTable() != SEP_NA) && (TraverseOprTable() != OPR_NA)) {
        // TraverseSepTable() moves 'curidx', need restore it
        curidx = old_pos + strlen(data->mData.mString);
        // Put into gStringPool
        gStringPool.FindString(data->mData.mString);
        found = true;
      } else {
        found = true;
      }
    }

    // If not found, restore curidx
    if (!found)
      curidx = old_pos;

    break;
  }

  // An example of this case is a rule of Decimal FP literal, like 12.5f.
  // rule DecFPLiteral contains '.' which is a token. The tokens seen here
  // are all system tokens.
  case DT_Token: {
    Token *token = &gSystemTokens[data->mData.mTokenId];
    found = MatchToken(token);
    break;
  }

  case DT_Subtable: {
    RuleTable *t = data->mData.mEntry;
    found = Traverse(t);

    if (!found)
      curidx = old_pos;

    break;
  }

  case DT_Type:
  case DT_Null:
  default:
    break;
  }

  return found;
}

// 1. Returns true if match succ, or false.
// 2. move curidx if succ.
bool Lexer::MatchToken(Token *token) {
  unsigned old_idx = curidx;
  bool found = false;

  switch (token->mTkType) {
  case TT_OP: {
    // Pick the longest matching operator.
    unsigned longest = 0;
    for (unsigned i = 0; i < OprTableSize; i++) {
      OprTableEntry e = OprTable[i];
      if ((e.mId == token->GetOprId()) &&
          !strncmp(line + curidx, e.mText, strlen(e.mText))) {
        found = true;
        unsigned len = strlen(e.mText);
        longest = len > longest ? len : longest;
      }
    }

    if (found)
      curidx += longest;
    break;
  }

  case TT_SP: {
    // Pick the longest matching separator.
    unsigned longest = 0;
    for (unsigned i = 0; i < SepTableSize; i++) {
      SepTableEntry e = SepTable[i];
      if ((e.mId == token->GetSepId()) &&
          !strncmp(line + curidx, e.mText, strlen(e.mText))) {
        found = true;
        unsigned len = strlen(e.mText);
        longest = len > longest ? len : longest;
      }
    }

    if (found)
      curidx += longest;
    break;
  }

  default:
    MASSERT(0 && "NIY: Not support token type in Lexer");
    break;
  }

  return found;
}

static bool IsZeroorxxxTableData (const TableData *td) {
  bool is_zero_xxx = false;
  switch (td->mType) {
  case DT_Subtable: {
    const RuleTable *t = td->mData.mEntry;
    EntryType type = t->mType;
    if ((type == ET_Zeroorone) || (type == ET_Zeroormore))
      is_zero_xxx = true;
    break;
  }
  default:
    break;
  }
  return is_zero_xxx;
}

bool Lexer::TraverseSecondTry(const RuleTable *rule_table) {
  // save the status
  char *old_line = line;
  unsigned old_pos = curidx;

  bool need_try = false;

  EntryType type = rule_table->mType;
  MASSERT((type == ET_Concatenate) && "Wrong entry type of table.");

  // Step 1. We only handle pattens ending with ... + ZEROORXXX(...) + YYY
  //         Anything else will return false.
  //         So I'll check the condition.
  unsigned i = 0;
  for (; i < rule_table->mNum; i++) {
    TableData *data = rule_table->mData + i;
    if (IsZeroorxxxTableData(data))
      break;
  }
  if (i != rule_table->mNum - 2)
    MERROR("Unsupported lex rule of SecondTry!");

  i = 0;
  bool found = false;

  // step 2. Let the parts before the two last sub-table to finish.
  //         If those fail, we don't need go on.
  for (; i < rule_table->mNum - 2; i++) {
    TableData *data = rule_table->mData + i;
    found = TraverseTableData(data);
    if (!found)
      break;
  }
  if (!found)
    return false;

  // step 3. The working loop.
  //         NOTE: 1. We are lucky here since it's lexing, so don't need cross the line.
  //               2. curidx points to the part for the ending two element.
  //               3. We need find the longest match.

  // These four are the final result if success.
  // [NOTE] The reason I use 'the one after' as xxx_end, is to check if TraverseTableData()
  // really moves the curidx. Or in another word, if it matches anything or nother.
  // If it matches something, xxx_end will be greater than xxx_start, or else they are equal.
  unsigned yyy_start = curidx;
  unsigned yyy_end = 0;             // the one after last char
  unsigned zeroxxx_start = curidx;  // A fixed index
  unsigned zeroxxx_end = 0;         // the one after last char

  // These four are the working result.
  unsigned w_yyy_start = curidx;
  unsigned w_yyy_end = 0;             // the one after last char
  unsigned w_zeroxxx_start = curidx;  // A fixed index
  unsigned w_zeroxxx_end = 0;         // the one after last char,

  TableData *zeroxxx = rule_table->mData + i;
  TableData *yyy = rule_table->mData + (i + 1);

  found = false;

  while (1) {
    // step 3.1 try yyy first.
    line = old_line;
    curidx = w_yyy_start;

    bool temp_found = false;
    while(!temp_found && curidx < current_line_size) {
      temp_found = TraverseTableData(yyy);
      if (!temp_found) {
        w_yyy_start++;
        curidx = w_yyy_start;
      }
    }

    if (!temp_found)
      break;
    else
      w_yyy_end = curidx;

    // step 3.2 try zeroxxx
    //          Have to build a line for the lexer.
    unsigned len = w_yyy_start - w_zeroxxx_start;
    if (len) {
      char *newline = (char*)malloc(len+1);
      strncpy(newline, line + w_zeroxxx_start, len);
      newline[len] = '\0';
      line = newline;
      curidx = 0;

      // 1. It always get true since it's a zeroxxx
      // 2. It may match nothing, meaning curidx never get a chance to move one step.
      temp_found = TraverseTableData(zeroxxx);
      MASSERT(temp_found && "Zeroxxx didn't return true.");
      free(newline);

      w_zeroxxx_end = curidx;
    }

    // step 3.3 Need check if zeroxxx_end is connected to yyy_start
    if ((w_zeroxxx_end + zeroxxx_start) == w_yyy_start) {
      found = true;
      yyy_start = w_yyy_start;
      yyy_end = w_yyy_end;
      //zeroxxx_start = w_zeroxxx_start; It's a fixed number
      zeroxxx_end = w_zeroxxx_end;
    } else
      break;

    // move yyy start one step
    w_yyy_start++;
  }

  // adjust the status
  if (found) {
    line = old_line;
    curidx = yyy_end;
    return true;
  } else {
    line = old_line;
    curidx = old_pos;
    return false;
  }
}

bool Lexer::Traverse(const RuleTable *rule_table) {

  // CHAR, DIGIT are reserved rules. It should NOT be changed. We can
  // expediate the lexing.
  if (rule_table == &TblCHAR) {
    char c = *(line + curidx);
    if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      curidx += 1;
      return true;
    } else {
      return false;
    }
  }

  if (rule_table == &TblDIGIT) {
    char c = *(line + curidx);
    if(c >= '0' && c <= '9') {
      curidx += 1;
      return true;
    } else {
      return false;
    }
  }

  bool matched = false;

  // save the original location
  unsigned old_pos = curidx;

  // Look into rule_table's data
  EntryType type = rule_table->mType;

  switch(type) {
  case ET_Oneof: {
    // We need find the longest match.
    bool found = false;
    unsigned new_pos = curidx;
    for (unsigned i = 0; i < rule_table->mNum; i++) {
      TableData *data = rule_table->mData + i;
      // Need restore curidx for the next try.
      curidx = old_pos;
      bool temp_found = TraverseTableData(data);
      if (temp_found) {
        found = true;
        if (curidx > new_pos) {
          new_pos = curidx;
          // Need set the literal type in order to make it easier
          // to ProcessLiteral().
          if (data->mType == DT_Subtable && rule_table == &TblLiteral) {
            if (data->mData.mEntry == &TblIntegerLiteral)
              mLastLiteralId = LT_IntegerLiteral;
            else if (data->mData.mEntry == &TblFPLiteral)
              mLastLiteralId = LT_FPLiteral;
            else if (data->mData.mEntry == &TblBooleanLiteral)
              mLastLiteralId = LT_BooleanLiteral;
            else if (data->mData.mEntry == &TblCharacterLiteral)
              mLastLiteralId = LT_CharacterLiteral;
            else if (data->mData.mEntry == &TblStringLiteral)
              mLastLiteralId = LT_StringLiteral;
            else if (data->mData.mEntry == &TblNullLiteral)
              mLastLiteralId = LT_NullLiteral;
          }
        }
      }
    }

    curidx = new_pos;
    matched = found; 
    break;
  }

  // Lexer moves until hit a NON-target data
  // It always return true because it doesn't matter how many target hit.
  case ET_Zeroormore: {
    matched = true;
    while(1) {
      bool found = false;
      MASSERT(rule_table->mNum == 1);
      TableData *data = rule_table->mData;
      found = TraverseTableData(data);
      if (!found)
        break;
    }
    break;
  }

  // It always matched. The lexer will stop after it zeor or at most one target
  case ET_Zeroorone: {
    matched = true;
    bool found = false;
    MASSERT(rule_table->mNum == 1);
    TableData *data = rule_table->mData;
    found = TraverseTableData(data);
    break;
  }

  case ET_Concatenate: {
    bool found = false;
    for (unsigned i = 0; i < rule_table->mNum; i++) {
      TableData *data = rule_table->mData + i;
      found = TraverseTableData(data);
      // The first element missed, then we stop.
      if (!found)
        break;
    }

    if (!found) {
      curidx = old_pos;
      if (rule_table->mProperties & RP_SecondTry)
        found = TraverseSecondTry(rule_table);
    }

    matched = found;
    break;
  }

  // Next table
  case ET_Data: {
    break;
  }

  case ET_Null:
  default: {
    break;
  }
  }

  if(matched) {
    return true;
  } else {
    curidx = old_pos;
    return false;
  }
}

// Return the separator ID, if it's. Or SEP_NA.
// Assuming the separator table has been sorted so as to catch the longest separator
//   if possible.
SepId Lexer::TraverseSepTable() {
  unsigned len = 0;
  SepId id = FindSeparator(line + curidx, 0, len);
  if (id != SEP_NA) {
    curidx += len;
    return id;
  }
  return SEP_NA;
}

// Returen the operator ID, if it's. Or OPR_NA.
// Assuming the operator table has been sorted so as to catch the longest separator
//   if possible.
OprId Lexer::TraverseOprTable() {
  unsigned len = 0;
  OprId id = FindOperator(line + curidx, 0, len);
  if (id != OPR_NA) {
    curidx += len;
    return id;
  }
  return OPR_NA;
}

// Return the keyword name, or else NULL.
const char* Lexer::TraverseKeywordTable() {
  unsigned len = 0;
  const char *addr = FindKeyword(line + curidx, 0, len);
  if (addr) {
    unsigned saved_curidx = curidx;

    // It's a keyword if the following is a separator
    // End of current line is a separator too.
    curidx += len;
    if ((current_line_size == curidx) || (TraverseSepTable() != SEP_NA)) {
      // TraverseSepTable() moves 'curidx', need move it back to after keyword
      curidx = saved_curidx + len;
      addr = gStringPool.FindString(addr);
      return addr;
    }

    // It's a keyword if the following is a operator
    curidx = saved_curidx + len;
    if ((TraverseOprTable() != OPR_NA)) {
      curidx = saved_curidx + len;
      addr = gStringPool.FindString(addr);
      return addr;
    }

    // failed, restore curidx
    curidx = saved_curidx;
  }
  return NULL;
}
}
