////////////////////////////////////////////////////////////////////////
// The lexical translation of the characters creates a sequence of input
// elements -----> white-space
//            |--> comments
//            |--> tokens --->identifiers
//                        |-->keywords
//                        |-->literals
//                        |-->separators
//                        |-->operators
//
// This categorization is shared among all languages. [NOTE] If anything
// in a new language is exceptional, please add to this.
//
// This file defines Tokens and all the sub categories
////////////////////////////////////////////////////////////////////////

#ifndef __Token_H__
#define __Token_H__

class Lexer;   // early decl, gen_separator.h needs it

#include <vector>
#include "element.h"
#include "stringutil.h"
#include "ruletable.h"

// TokenText contains the text data of each token, no matter it's a
// number or string. TokenText will be further processed by utility
// functions before creating a real token.
typedef std::vector<std::string> TokenText;

typedef enum {
  TT_ID,    // Identifier
  TT_KW,    // Keyword
  TT_LT,    // Literal
  TT_SP,    // separator
  TT_OP,    // operator
  TT_NA     // N.A.
}TK_Type;

class Token : public Element {
public:
  TK_Type mTkType;
public:
  Token(TK_Type t) : mTkType(t) {EType = ET_TK;}
  Token(){EType = ET_TK;}
  virtual void Dump(){}
};

////////////////////////////////////////////////////////////////////////
//                       IdentifierToken                              //
////////////////////////////////////////////////////////////////////////
class IdentifierToken : public Token {
public:
  const char *mName;       // It's put into the Lexer's StringPool
public:
  IdentifierToken(const char *s) : mName(s) {mTkType = TT_ID;}
  IdentifierToken() : mName(NULL) {mTkType = TT_ID;}

  void Dump();
};

////////////////////////////////////////////////////////////////////////
//                            Keywod                                  //
// Keywords fall into different categories:
//   1. data type, including class and interface
//   2. control flow
//   3. attributes, for primitive and Object data
////////////////////////////////////////////////////////////////////////
//
typedef enum {
  KW_DT,     // data type, class, enum, interfaces, are all considered types
  KW_FN,     // Intrinsic call, system call, ...
  KW_CT,     // control flow keyword,
  KW_DA,     // data attribute,
  KW_FA,     // function attribute
  KW_OA,     // object(class,enum) attribute
  KW_SP,     // Language Sepcific
  KW_UN      // Undefined
}KW_Type;

#define KEYWORD(N,I,T) KW_ID_##I,
typedef enum {
#include "keywords.def"
  KW_ID_NULL
}KW_ID;

class KeywordToken : public Token {
public:
  const char *mName;   // The text name. During initialization it will be
                       // put into string pool.
public:
  KeywordToken() : Token(TT_KW){ mName = NULL; }
  KeywordToken(const char *s) : Token(TT_KW), mName(s){}
  
  void SetName(const char *s){ mName = s; }
  void Dump();
};

////////////////////////////////////////////////////////////////////////
//                       LiteralToken                                 //
// The literals defined here are from autogen/supported_literals.def  //
// Autogen generates GenIntegerLiteral(...) and etc. which recognize  //
// LT_Int and the correspondings.                                     //
////////////////////////////////////////////////////////////////////////
#define LITERAL(T) LT_##T,
typedef enum {
#include "supported_literals.def"
  LT_NA     // N/A, in java, Null is legal type with only one value 'null'
            // reference, a literal. So LT_Null is actually legal. 
            // So I put LT_NA for the illegal literal
}LT_Type;

struct LitData{
  LT_Type mType; 
  union {
    int   mInt;
    float mFloat;
    bool  mBool;
    char  mChar;
    char *mStr;   // the string is allocated through lexer's mStringPool
  }mData;
};

class LiteralToken : public Token {
private:
  LitData  mData;
public:
  LiteralToken(LitData data) {mTkType = TT_LT; mData = data;}
};

#include "gen_literal.h"

////////////////////////////////////////////////////////////////////////
//                       SeparatorToken                               //
////////////////////////////////////////////////////////////////////////

class SeparatorToken : public Token {
public:
  SepId mSepId;
public:
  SeparatorToken(SepId si) {mTkType = TT_SP; mSepId = si;}

  const char* GetName();
  void Dump();
};

////////////////////////////////////////////////////////////////////////
//                              Operator                              //
////////////////////////////////////////////////////////////////////////
class Operator : public Token {
public:
  Operator() {mTkType = TT_OP;}
};

#endif
