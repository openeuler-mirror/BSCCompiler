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
#include "lang_spec.h"
#include "stringpool.h"

namespace maplefe {

// For all the string to value functions below, we assume the syntax of 's' is correct
// for a literal in Java.

float  StringToValueImpl::StringToFloat(std::string &s) {
  return stof(s);
}

// Java use 'd' or 'D' as double suffix. C++ use 'l' or 'L'.
double StringToValueImpl::StringToDouble(std::string &s) {
  std::string str = s;
  char suffix = str[str.length() - 1];
  if (suffix == 'd' || suffix == 'D')
    str[str.length() - 1] = 'L';
  return stod(str);
}

bool StringToValueImpl::StringToBool(std::string &s) {
  if ((s.size() == 4) && (s.compare("true") == 0))
    return true;
  else if ((s.size() == 5) && (s.compare("false") == 0))
    return false;
  else
    MERROR("unknown bool literal");
}

bool StringToValueImpl::StringIsNull(std::string &s) {return false;}

const char* StringToValueImpl::StringToString(std::string &in_str) {
  std::string target;

  // For most languages, the input 'in_str' still contains the leading " or ' and the
  // ending " or '. They need to be removed.
  std::string str;

  // If empty string literal, return the empty 'target'.
  if (in_str.size() == 2) {
    const char *s = gStringPool.FindString(target);
    return s;
  } else {
    str.assign(in_str, 1, in_str.size() - 2);
  }

  // For typescript, if a string literal is:
  //    s : string = "abc \
  //                   efg";
  // The \ is actually connnecting the next line into the string literal.
  // We need handle the connection.

  std::string s_ret;
  for (unsigned i = 0; i < str.length(); i++) {
    char c = str[i];
    if (c == '\\') {
      if ((i < str.length() - 1) && (str[i+1] == '\n')) {
        // skip \ and \n
        i += 1;
        continue;
      }
    }
    s_ret.push_back(c);
  }

  const char *s = gStringPool.FindString(s_ret);
  return s;
}

static char DeEscape(char c) {
  switch(c) {
  case 'b':
    return '\b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  case '"':
    return '\"';
  case '\'':
    return '\'';
  case '\\':
    return '\\';
  case '0':
    return '\0';
  default:
    MERROR("Unsupported in DeEscape().");
  }
}

static int char2int(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    MERROR("Unsupported char in char2int().");
}

Char StringToValueImpl::StringToChar(std::string &s) {
  Char ret_char;
  ret_char.mIsUnicode = false;
  MASSERT (s[0] == '\'');
  if (s[1] == '\\') {
    if (s[2] == 'u') {
      ret_char.mIsUnicode = true;
      int first = char2int(s[3]);
      int second = char2int(s[4]);
      int third = char2int(s[5]);
      int forth = char2int(s[6]);
      MASSERT(s[7] == '\'');
      ret_char.mData.mUniValue = (first << 12) + (second << 8)
                               + (third << 4) + forth;
    } else {
      ret_char.mData.mChar = DeEscape(s[2]);
    }
  } else {
    MASSERT(s[2] == '\'');
    ret_char.mData.mChar = s[1];
  }
  return ret_char;
}

// Each language has its own format of literal. So this function handles Typescript literals.
// It translate a string into a literal.
//
// 'str' is in the Lexer's string pool.
//
LitData ProcessLiteral(LitId id, const char *str) {
  LitData data;
  std::string value_text(str);
  StringToValueImpl s2v;

  switch (id) {
  case LT_IntegerLiteral: {
    long l = s2v.StringToLong(value_text);
    data.mType = LT_IntegerLiteral;
    data.mData.mInt = l;
    break;
  }
  case LT_FPLiteral: {
    // Java spec doesn't define rules for double. Both float and double
    // are covered by Float Point. But we need differentiate here.
    // Check if it's a float of double. Non-suffix means double.
    char suffix = value_text[value_text.length() - 1];
    if (suffix == 'f' || suffix == 'F') {
      float f = s2v.StringToFloat(value_text);
      data.mType = LT_FPLiteral;
      data.mData.mFloat = f;
    } else {
      double d = s2v.StringToDouble(value_text);
      data.mType = LT_DoubleLiteral;
      data.mData.mDouble = d;
    }
    break;
  }
  case LT_BooleanLiteral: {
    bool b = s2v.StringToBool(value_text);
    data.mType = LT_BooleanLiteral;
    data.mData.mBool = b;
    break; }
  case LT_CharacterLiteral: {
    Char c = s2v.StringToChar(value_text);
    data.mType = LT_CharacterLiteral;
    data.mData.mChar = c;
    break; }
  case LT_StringLiteral: {
    const char *s = s2v.StringToString(value_text);
    data.mType = LT_StringLiteral;
    data.mData.mStrIdx = gStringPool.GetStrIdx(s);
    break; }
  case LT_NullLiteral: {
    // Just need set the id
    data.mType = LT_NullLiteral;
    break; }
  case LT_NA:    // N/A,
  default:
    data.mType = LT_NA;
    break;
  }

  return data;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//                   Implementation of typescript Lexer
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

bool TypescriptLexer::CharIsSeparator(const char c) {
  if (c == '`')
    return true;
  return false;
}

// NOTE: right now we rely on 'tsc' to assure the input is legal,
//       so I'll make many things easier and will skip many lexical
//       checks. Just make it easy for now.
//       Also, I assume we don't handle multiple line template literal
//       for the time being.
TempLitData* TypescriptLexer::GetTempLit() {
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
      std::string fmt_str = "";
      bool s_found = FindNextTLFormat(start_idx, fmt_str, end_idx);
      const char *addr = NULL;
      if (s_found) {
        MASSERT(fmt_str.size() > 0 && "found token has 0 data?");
        addr = gStringPool.FindString(fmt_str);
        start_idx = end_idx + 1;
      }

      // Try pattern
      end_idx = 0;
      const char *addr_ph = NULL;
      std::string pl_str = "";
      bool p_found = FindNextTLPlaceHolder(start_idx, pl_str, end_idx);
      if (p_found) {
        unsigned len = pl_str.size();
        MASSERT(len > 0 && "found token has 0 data?");
        addr_ph = gStringPool.FindString(pl_str);
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
bool TypescriptLexer::FindNextTLFormat(unsigned start_idx, std::string &str, unsigned& end_idx) {
  unsigned working_idx = start_idx;
  while(1) {
    if ((line[working_idx] == '$' && line[working_idx+1] == '{')
        || line[working_idx] == '`' ){
      end_idx = working_idx - 1;
      break;
    }

    // Template Literal allows \n in format and place holder.
    // the \n was removed by Lexer in the beginning of ReadALine();
    if (working_idx == current_line_size) {
      str += '\n';
      ReadALine();
      if (endoffile)
        return false;
      working_idx = 0;
      continue;
    }

    str += line[working_idx];
    working_idx++;
  }

  if (str.size() > 0)
    return true;
  else
    return false;
}

// Find the pattern string of a template literal.
// Set end_idx as the last char of string.
//
// [NOTE] For nested template literals, we will matching the outmost
//        template literal and treat the inner template literal as
//        a plain string. The innter template literal will later
//        be handled by the parsing of the outmost template literal.
//        This makes things easier.
//
// [NOTE] We only support two level of nesting temp lit !!

bool TypescriptLexer::FindNextTLPlaceHolder(unsigned start_idx, std::string& str, unsigned& end_idx) {
  unsigned working_idx = start_idx;
  if (line[working_idx] != '$' || line[working_idx+1] != '{')
    return false;

  working_idx = start_idx + 2;

  // There could be {..} inside placeholder.
  unsigned num_left_brace = 0;

  // There could be string literal inside placeholder,
  bool in_string_literal = false;

  // There could be nested template literal inside a place holder.
  bool in_nested_temp_lit = false;
  bool waiting_right_brace = false;
  unsigned num_left_brace_inner = 0; // there could be {..} for inner temp lit.

  while(1) {

    if (line[working_idx] == '`') {
      if (in_nested_temp_lit) {
        // finish inner temp lit. Need clear the status.
        in_nested_temp_lit = false;
        num_left_brace_inner = 0;
      } else {
        in_nested_temp_lit = true;
      }
    } else if (line[working_idx] == '$' && line[working_idx+1] == '{') {
      MASSERT(in_nested_temp_lit);
      str += line[working_idx];
      str += line[working_idx + 1];
      working_idx += 2;
      waiting_right_brace = true;
      continue;
    } else if (line[working_idx] == '\'' || line[working_idx] == '\"') {
      in_string_literal = in_string_literal ? false : true;
    } else if (line[working_idx] == '{') {
      if (!in_string_literal) {
        if (in_nested_temp_lit)
          num_left_brace_inner++;
        else
          num_left_brace++;
      }
    } else if (line[working_idx] == '}') {
      if (!in_string_literal) {
        if (waiting_right_brace)
          waiting_right_brace = false;
        else if (!in_nested_temp_lit && num_left_brace > 0)
          num_left_brace--;
        else if (in_nested_temp_lit && num_left_brace_inner > 0)
          num_left_brace_inner--;
        else
          break;
      }
    }

    // Template Literal allows \n in format and place holder.
    // the \n was removed by Lexer in the beginning of ReadALine();
    // 
    // I don't need worry about if the template literal is ended or not,
    // since tsc guarantees it's correct.
    if (working_idx == current_line_size) {
      str += '\n';
      ReadALine();
      if (endoffile)
        return false;
      working_idx = 0;
    } else {
      str += line[working_idx];
      working_idx++;
    }
  }

  end_idx = working_idx - 1;
  return true;
}

Lexer* CreateLexer() {
  Lexer *lexer = new TypescriptLexer();
  return lexer;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//                   Implementation of typescript Parser
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

// Return a reg expr token instead of t if t is the beginning of a regular expression.
// Or return t.
Token* TypescriptParser::GetRegExpr(Token *t) {
  if (!t->IsOperator())
    return t;

  if (t->GetOprId() != OPR_Div)
    return t;

  unsigned size = mActiveTokens.GetNum();
  if (size < 2)
    return t;

  // We take care of only the following scenarios.
  // If more need support, we will add later.
  //   (/abc*/g, )
  //   [/abc*/g, ]
  //   =/abc*/g;
  //   &&  /abc*/g;
  //   ,/abc*/g;
  //   : /abc*/g;
  //   || /abc*/g;

  Token *sep = mActiveTokens.ValueAtIndex(size - 1);
  bool is_sep = false;
  if (sep->IsSeparator() && (sep->GetSepId() == SEP_Lparen))
    is_sep = true;
  if (sep->IsSeparator() && (sep->GetSepId() == SEP_Lbrack))
    is_sep = true;
  if (sep->IsSeparator() && (sep->GetSepId() == SEP_Comma))
    is_sep = true;
  if (sep->IsSeparator() && (sep->GetSepId() == SEP_Colon))
    is_sep = true;
  if (sep->IsOperator() && (sep->GetOprId() == OPR_Assign))
    is_sep = true;
  if (sep->IsOperator() && (sep->GetOprId() == OPR_Land))
    is_sep = true;
  if (sep->IsOperator() && (sep->GetOprId() == OPR_Lor))
    is_sep = true;
  if (!is_sep)
    return t;

  Token *regexpr = mLexer->FindRegExprToken();
  if (regexpr)
    t = regexpr;

  return t;
}

// return true if t should be split into multiple tokens.
// [NOTE] t is not push into mActiveTokens yet.
//
// We will handle these cases specifically.
//
// We take care of only one scenarios right now..
//   typename<typearg>= initval
// Look at the '>='. It first recognazied by lexer as GE,
// but it's actually a > and a =.
//
// Another case is
//   typename<t extends T>= s;

bool TypescriptParser::TokenSplit(Token *t) {
  if (!t->IsOperator() || t->GetOprId() != OPR_GE)
    return false;
  unsigned size = mActiveTokens.GetNum();
  if (size < 2)
    return false;

  Token *type_arg = mActiveTokens.ValueAtIndex(size - 1);
  if (!type_arg->IsIdentifier())
    return false;

  Token *extends_token = FindKeywordToken("extends");

  Token *lt = mActiveTokens.ValueAtIndex(size - 2);

  if (lt->Equal(extends_token)) {
    // This is a good candidate. Do nothing
  } else {
    if (!lt->IsOperator() || lt->GetOprId() != OPR_LT)
      return false;

    Token *type_name = mActiveTokens.ValueAtIndex(size - 3);
    if (!type_name->IsIdentifier())
      return false;
  }

  // Now we got a matching case.
  Token *gt_token = FindOperatorToken(OPR_GT);
  Token *assign_token = FindOperatorToken(OPR_Assign);
  mActiveTokens.PushBack(gt_token);
  mActiveTokens.PushBack(assign_token);

  if (mLexer->mTrace) {
    std::cout << "Split >= to > and =" << std::endl;
  }

  return true;
}


// 'appeal' is the node of 'rule_table'.
// 'child' was NULL when passed in.
bool TypescriptParser::TraverseASI(RuleTable *rule_table,
                                   AppealNode *appeal,
                                   AppealNode *&child) {
  // Usually mCurToken is a new token to be matched. So if it's end of file, we simply return false.
  // However, (1) if mCurToken is actually an ATMToken, which means it needs to be matched
  //              multiple times, we are NOT at the end yet.
  //          (2) If we are traverse a Concatenate rule, and the previous sub-rule has multiple matches,
  //              and we are trying the current sub-rule, ie. 'data', using one of the matches.
  //              The lexer actually reaches the EndOfFile in previous matchings, but the mCurToken
  //              we are working on right now is not the last token. It's one of the previous matches.
  // So we need check if we are matching the last token.
  //if (mEndOfFile && mCurToken >= mActiveTokens.GetNum()) {
  //  if (!(mInAltTokensMatching && (mCurToken == mATMToken)))
  //    return false;
  //}

  if (mCurToken <= 1)
    return false;

  if (mEndOfFile && mCurToken == mActiveTokens.GetNum())
    return true;

  unsigned old_pos = mCurToken;
  bool     found = false;
  Token   *curr_token = GetActiveToken(mCurToken);
  Token   *prev_token = GetActiveToken(mCurToken - 1);

  MASSERT((rule_table->mNum == 1) && "ASI node has more than one elements?");

  TableData *data = rule_table->mData;
  MASSERT(data->mType == DT_Token && "ASI data is not a token?");

  Token *semicolon = &gSystemTokens[data->mData.mTokenId];
  MASSERT(semicolon->IsSeparator());
  MASSERT(semicolon->GetSepId() == SEP_Semicolon);

  if (curr_token->Equal(semicolon)) {
    // To simplify the code, I reused TraverseToken().
    found = TraverseToken(semicolon, appeal, child);
  } else {
    // case 1. We are crossing lines.
    if (curr_token->mLineBegin && prev_token->mLineEnd) {
      // If prev token (line end) is a separator
      if (prev_token->IsSeparator() &&
           (prev_token->GetSepId() == SEP_Rbrace ||
            prev_token->GetSepId() == SEP_Rbrack ||
            prev_token->GetSepId() == SEP_Rparen)) {
        if (mTraceTable) {
          std::cout << "TraverseASI, Auto-insert one semicolon." << std::endl;
        }
        found = true;
      }

      // if prev token is identifier or keyword AND the curr token is also id or keyword
      // we can try to insert semicolon. A simple case is:
      //        a    <-- we can insert semicolon
      //        console.log();
      if ( (prev_token->IsIdentifier() || prev_token->IsKeyword()) &&
           (curr_token->IsIdentifier() || curr_token->IsKeyword()) ){
        if (mTraceTable) {
          std::cout << "TraverseASI, Auto-insert one semicolon." << std::endl;
        }
        found = true;
      }

      if (found)
        return found;
    }

    // case 2. like:
    //     {foo()}   <-- , is missed before }
    if (curr_token->IsSeparator() && (curr_token->GetSepId() == SEP_Rbrace))
      return true;
  }

  if (child) {
    child->SetChildIndex(0);
    appeal->CopyMatch(child);
  }

  return found;
}

}
