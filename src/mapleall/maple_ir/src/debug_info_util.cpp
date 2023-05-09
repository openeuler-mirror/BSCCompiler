/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.
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

#include "mir_builder.h"
#include "debug_info.h"
#include "global_tables.h"
#include "mir_type.h"

namespace maple {
#define TOSTR(s) #s
// utility functions to get the string from tag value etc.
// GetDwTagName(unsigned n)
const char *GetDwTagName(unsigned n) {
  switch (n) {
#define DW_TAG(ID, NAME) case DW_TAG_##NAME: return TOSTR(DW_TAG_##NAME);
#include "dwarf.def"
    case DW_TAG_lo_user: return "DW_TAG_lo_user";
    case DW_TAG_hi_user: return "DW_TAG_hi_user";
    case DW_TAG_user_base: return "DW_TAG_user_base";
    default: return nullptr;
  }
}

// GetDwFormName(unsigned n)
const char *GetDwFormName(unsigned n) {
  switch (n) {
#define DW_FORM(ID, NAME) case DW_FORM_##NAME: return TOSTR(DW_FORM_##NAME);
#include "dwarf.def"
    case DW_FORM_lo_user: return "DW_FORM_lo_user";
    default: return nullptr;
  }
}

// GetDwAtName(unsigned n)
const char *GetDwAtName(unsigned n) {
  switch (n) {
#define DW_AT(ID, NAME)  case DW_AT_##NAME: return TOSTR(DW_AT_##NAME);
#include "dwarf.def"
    case DW_AT_lo_user: return "DW_AT_lo_user";
    default: return nullptr;
  }
}

// GetDwOpName(unsigned n)
const char *GetDwOpName(unsigned n) {
  switch (n) {
#define DW_OP(ID, NAME) case DW_OP_##NAME: return TOSTR(DW_OP_##NAME);
#include "dwarf.def"
    case DW_OP_hi_user: return "DW_OP_hi_user";
    default: return nullptr;
  }
}

#define DW_ATE_void 0x20
// GetDwAteName(unsigned n)
const char *GetDwAteName(unsigned n) {
  switch (n) {
#define DW_ATE(ID, NAME) case DW_ATE_##NAME: return TOSTR(DW_ATE_##NAME);
#include "dwarf.def"
    case DW_ATE_lo_user: return "DW_ATE_lo_user";
    case DW_ATE_hi_user: return "DW_ATE_hi_user";
    case DW_ATE_void: return "DW_ATE_void";
    default: return nullptr;
  }
}

DwAte GetAteFromPTY(PrimType pty) {
  switch (pty) {
    case PTY_u1:
      return DW_ATE_boolean;
    case PTY_u8:
      return DW_ATE_unsigned_char;
    case PTY_u16:
    case PTY_u32:
    case PTY_u64:
      return DW_ATE_unsigned;
    case PTY_i8:
      return DW_ATE_signed_char;
    case PTY_i16:
    case PTY_i32:
    case PTY_i64:
      return DW_ATE_signed;
    case PTY_f32:
    case PTY_f64:
    case PTY_f128:
      return DW_ATE_float;
    case PTY_agg:
    case PTY_ref:
    case PTY_ptr:
    case PTY_a32:
    case PTY_a64:
      return DW_ATE_address;
    case PTY_c64:
    case PTY_c128:
      return DW_ATE_complex_float;
    case PTY_void:
      return DW_ATE_void;
    default:
      return DW_ATE_void;
  }
}
}  // namespace maple
