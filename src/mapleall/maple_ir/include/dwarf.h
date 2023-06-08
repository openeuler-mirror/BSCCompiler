/*
 * Copyright (C) [2022] Futurewei Technologies, Inc. All rights reverved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLE_IR_INCLUDE_DWARF_H
#define MAPLE_IR_INCLUDE_DWARF_H

#include <cstdint>

enum Tag : uint16_t {
#define DW_TAG(ID, NAME) DW_TAG_##NAME = (ID),
#include "dwarf.def"
  DW_TAG_lo_user = 0x4080,
  DW_TAG_hi_user = 0xffff,
  DW_TAG_user_base = 0x1000
};

enum Attribute : uint16_t {
#define DW_AT(ID, NAME) DW_AT_##NAME = (ID),
#include "dwarf.def"
  DW_AT_lo_user = 0x2000,
  DW_AT_hi_user = 0x3fff,
};

enum Form : uint16_t {
#define DW_FORM(ID, NAME) DW_FORM_##NAME = (ID),
#include "dwarf.def"
  DW_FORM_lo_user = 0x1f00,
};

enum LocationAtom {
#define DW_OP(ID, NAME) DW_OP_##NAME = (ID),
#include "dwarf.def"
  DW_OP_lo_user = 0xe0,
  DW_OP_hi_user = 0xff,
};

enum TypeKind : uint8_t {
#define DW_ATE(ID, NAME) DW_ATE_##NAME = (ID),
#include "dwarf.def"
  DW_ATE_lo_user = 0x80,
  DW_ATE_hi_user = 0xff,
  DW_ATE_void = 0x20
};

enum AccessAttribute {
  DW_ACCESS_public = 0x01,
  DW_ACCESS_protected = 0x02,
  DW_ACCESS_private = 0x03
};


enum SourceLanguage {
#define DW_LANG(ID, NAME, LOWER_BOUND) DW_LANG_##NAME = (ID),
#include "dwarf.def"
  DW_LANG_lo_user = 0x8000,
  DW_LANG_hi_user = 0xffff
};

#endif
