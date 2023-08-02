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
#ifndef MAPLEBE_MDGEN_INCLUDE_MDTOKENS_H
#define MAPLEBE_MDGEN_INCLUDE_MDTOKENS_H
enum MDTokenKind {
  /* special symbols */
  kMDError,
  kMDEOF,
  kMDInvalid,

  /* normal symbols */
  kMDOpenParen,           /* ( */
  kMDCloseParen,          /* ) */
  kMDOpenBrace,           /* { */
  kMDCloseBrace,          /* } */
  kMDOpenSquare,          /* [ */
  kMDCloseSquare,         /* ] */
  kMDEqual,               /* = */
  kMDSemi,                /* ; */
  kMDComma,               /* , */
  kMDColon,               /* : */
  kMDLess,                /* < */
  kMDGreater,             /* > */
  kMDLgAnd,               /* & */
  kMDLgOr,                /* | */

  kMDIdentifier,
  /* const values */
  kMDIntVal,
  kMDFloatVal,
  kMDDoubleVal,

  /* keywords */
  kMDDef,
  kMDClass,
  kMDAnonClass,
  kMDDefType,
};

#endif /* MAPLEBE_MDGEN_INCLUDE_MDTOKENS_H */