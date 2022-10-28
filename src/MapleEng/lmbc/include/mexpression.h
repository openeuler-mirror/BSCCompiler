/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#ifndef MPLENG_MEXPRESSION_H_
#define MPLENG_MEXPRESSION_H_

#include <cstdint>
#include "massert.h" // for MASSERT

#define EXPRBINOP(exprop, res, op0, op1, exprPtyp) \
    do { \
        switch(exprPtyp) { \
            case PTY_i8:  res.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: res.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: res.x.i32 = (int64)op0.x.i32 exprop (int64)op1.x.i32; break; \
            case PTY_i64: res.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u8:  res.x.u8  = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_u16: res.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u32: res.x.u32 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_a64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_f32: res.x.f32 = op0.x.f32 exprop op1.x.f32; break; \
            case PTY_f64: res.x.f64 = op0.x.f64 exprop op1.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary operator %s", exprPtyp, #exprop); \
        } \
        res.ptyp = expr->ptyp; \
    } while(0)

#define EXPRCOMPOP(exprop,res, op0, op1, optyp, exprptyp) \
    do { \
        /*MASSERT(op0.ptyp == op1.ptyp, "COMOP Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp);*/ \
        switch(optyp) { \
            case PTY_i8:  res.x.i64 = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: res.x.i64 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: res.x.i64 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: res.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u8:  res.x.i64 = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_u16: res.x.i64 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u32: res.x.i64 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u64: res.x.i64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_a64: res.x.i64 = op0.x.a64 exprop op1.x.a64; break; \
            case PTY_f32: res.x.i64 = op0.x.f32 exprop op1.x.f32; break; \
            case PTY_f64: res.x.i64 = op0.x.f64 exprop op1.x.f64; break; \
            default: MIR_FATAL("Unsupported operand PrimType %d for comparison operator %s", op0.ptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

// EXPRCOMPOP with f32 amd f64 comparison cases taken out because of 
// -Wfloat-equal compile flag warnings with != and == on float types.
// The float cases for != and == are special case handled in op handlers.
#define EXPRCOMPOPNOFLOAT(exprop,res, op0, op1, optyp, exprptyp) \
    do { \
        /*MASSERT(op0.ptyp == op1.ptyp, "COMPOP Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp);*/ \
        switch(optyp) { \
            case PTY_i8:  res.x.i64 = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: res.x.i64 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: res.x.i64 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: res.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u8:  res.x.i64 = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_u16: res.x.i64 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u32: res.x.i64 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u64: res.x.i64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_a64: res.x.i64 = op0.x.a64 exprop op1.x.a64; break; \
            default: MIR_FATAL("Unsupported operand PrimType %d for comparison operator %s", op0.ptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

#define EXPRSELECTOP(res, op0, sel1, sel2, exprptyp) \
    do { \
        MValue op1, op2;  \
        op1 = CvtType(sel1, exprptyp, sel1.ptyp);  \
        op2 = CvtType(sel2, exprptyp, sel2.ptyp);  \
        switch(exprptyp) { \
            case PTY_i8:  res.x.i8  = op0.x.i64? op1.x.i8  : op2.x.i8;  break; \
            case PTY_i16: res.x.i16 = op0.x.i64? op1.x.i16 : op2.x.i16; break; \
            case PTY_i32: res.x.i32 = op0.x.i64? op1.x.i32 : op2.x.i32; break; \
            case PTY_i64: res.x.i64 = op0.x.i64? op1.x.i64 : op2.x.i64; break; \
            case PTY_u8:  res.x.u8  = op0.x.i64? op1.x.u8  : op2.x.u8;  break; \
            case PTY_u16: res.x.u16 = op0.x.i64? op1.x.u16 : op2.x.u16; break; \
            case PTY_u32: res.x.u32 = op0.x.i64? op1.x.u32 : op2.x.u32; break; \
            case PTY_u64: res.x.u64 = op0.x.i64? op1.x.u64 : op2.x.u64; break; \
            case PTY_a64: res.x.a64 = op0.x.i64? op1.x.a64 : op2.x.a64; break; \
            case PTY_f32: res.x.f32 = op0.x.i64? op1.x.f32 : op2.x.f32; break; \
            case PTY_f64: res.x.f64 = op0.x.i64? op1.x.f64 : op2.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for select operator", exprptyp); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

#define EXPRBININTOP(exprop, res, op, op1, exprptyp) \
    do { \
        MValue op0 = CvtType(op, exprptyp, op.ptyp); \
        switch(exprptyp) { \
            case PTY_i8:  res.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: res.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: res.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: res.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u8:  res.x.u8  = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_u16: res.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u32: res.x.u32 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_a64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for integer binary operator %s", exprptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

// Used by OP_lshr only
#define EXPRBININTOPUNSIGNED(exprop, res, op0, op1, exprptyp) \
    do { \
        MASSERT((op0.ptyp == exprptyp) || \
                (op0.ptyp == PTY_u32 && exprptyp == PTY_i32) || \
                (op0.ptyp == PTY_i32 && exprptyp == PTY_u32), \
            "BINUINTOP Type mismatch: 0x%02x and 0x%02x", op0.ptyp, exprptyp); \
        switch(op1.ptyp) { \
            case PTY_i8:  \
            case PTY_u8:  \
              MASSERT(op1.x.u8 <= 64, "OP_lshr shifting more than 64 bites"); \
              break;      \
            case PTY_i16: \
            case PTY_u16: \
              MASSERT(op1.x.u16 <= 64, "OP_lshr shifting more than 64 bites"); \
              break;      \
            case PTY_i32: \
            case PTY_u32: \
              MASSERT(op1.x.u32 <= 64, "OP_lshr shifting more than 64 bites"); \
              break;      \
            case PTY_i64: \
            case PTY_u64: \
            case PTY_a64: \
              MASSERT(op1.x.u64 <= 64, "OP_lshr shifting more than 64 bites"); \
              break;      \
            default:      \
              MIR_FATAL("Unsupported PrimType %d for unsigned integer binary operator %s", exprptyp, #exprop); \
              break;      \
        } \
        switch(exprptyp) { \
            case PTY_i8:  res.x.u8  = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_i16: res.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_i32: res.x.u32 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_i64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_a64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            case PTY_u8:  res.x.u8  = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_u16: res.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u32: res.x.u32 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u64: res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for unsigned integer binary operator %s", exprptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

#define EXPRMAXMINOP(exprop, res, op0, op1, exprptyp) \
    do { \
        MASSERT(op0.ptyp == op1.ptyp, "MAXMINOP Type mismatch: 0x%02x and 0x%02x", op0.ptyp, op1.ptyp); \
        MASSERT(op0.ptyp == exprptyp, "MAXMINOP Type mismatch: 0x%02x and 0x%02x", op0.ptyp, exprptyp); \
        switch(exprptyp) { \
            case PTY_i8:  res.x.i8  = op0.x.i8  exprop op1.x.i8?  op0.x.i8  : op1.x.i8;  break; \
            case PTY_i16: res.x.i16 = op0.x.i16 exprop op1.x.i16? op0.x.i16 : op1.x.i16; break; \
            case PTY_i32: res.x.i32 = op0.x.i32 exprop op1.x.i32? op0.x.i32 : op1.x.i32; break; \
            case PTY_i64: res.x.i64 = op0.x.i64 exprop op1.x.i64? op0.x.i64 : op1.x.i64; break; \
            case PTY_u8:  res.x.u8  = op0.x.u8  exprop op1.x.u8 ? op0.x.u8  : op1.x.u8;  break; \
            case PTY_u16: res.x.u16 = op0.x.u16 exprop op1.x.u16? op0.x.u16 : op1.x.u16; break; \
            case PTY_u32: res.x.u32 = op0.x.u32 exprop op1.x.u32? op0.x.u32 : op1.x.u32; break; \
            case PTY_u64: res.x.u64 = op0.x.u64 exprop op1.x.u64? op0.x.u64 : op1.x.u64; break; \
            case PTY_a64: res.x.a64 = op0.x.a64 exprop op1.x.a64? op0.x.a64 : op1.x.a64; break; \
            case PTY_f32: res.x.f32 = op0.x.f32 exprop op1.x.f32? op0.x.f32 : op1.x.f32; break; \
            case PTY_f64: res.x.f64 = op0.x.f64 exprop op1.x.f64? op0.x.f64 : op1.x.f64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for binary max/min operator %s", exprptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

#define EXPRREMOP(exprop, res, op0, op1, exprptyp) \
    do { \
        switch(exprptyp) { \
            case PTY_i8:  if(op1.x.i8 == 0) res.x.i8 = 0; \
                              else if(op1.x.i8 == -1 && op0.x.i8 == INT8_MIN) op0.x.i8 = 0; \
                              else res.x.i8  = op0.x.i8  exprop op1.x.i8;  break; \
            case PTY_i16: if(op1.x.i16 == 0) res.x.i16 = 0; \
                              else if(op1.x.i16 == -1 && op0.x.i16 == INT16_MIN) op0.x.i16 = 0; \
                              else res.x.i16 = op0.x.i16 exprop op1.x.i16; break; \
            case PTY_i32: if(op1.x.i32 == 0) res.x.i32 = 0; \
                              else if(op1.x.i32 == -1 && op0.x.i32 == INT32_MIN) op0.x.i32 = 0; \
                              else res.x.i32 = op0.x.i32 exprop op1.x.i32; break; \
            case PTY_i64: if(op1.x.i64 == 0) res.x.i64 = 0; \
                              else if(op1.x.i64 == -1 && op0.x.i64 == INT64_MIN) op0.x.i64 = 0; \
                              else res.x.i64 = op0.x.i64 exprop op1.x.i64; break; \
            case PTY_u8:  if(op1.x.u8 == 0) res.x.u8 = 0; \
                              else res.x.u8  = op0.x.u8  exprop op1.x.u8;  break; \
            case PTY_u16: if(op1.x.u16 == 0) res.x.u16 = 0; \
                              else res.x.u16 = op0.x.u16 exprop op1.x.u16; break; \
            case PTY_u32: if(op1.x.u32 == 0) res.x.u32 = 0; \
                              else res.x.u32 = op0.x.u32 exprop op1.x.u32; break; \
            case PTY_u64: if(op1.x.u64 == 0) res.x.u64 = 0; \
                              else res.x.u64 = op0.x.u64 exprop op1.x.u64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for rem operator %s", exprptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

#define EXPRUNROP(exprop, res, op0, exprptyp) \
    do { \
        MASSERT(op0.ptyp == exprptyp || \
                ((op0.ptyp == PTY_i32 || op0.ptyp == PTY_u32) && \
                 (exprptyp == PTY_i32 || exprptyp == PTY_u32)) , "UNROP Type mismatch: 0x%02x and 0x%02x", op0.ptyp, exprptyp); \
        switch(exprptyp) { \
            case PTY_i8:  res.x.i8  = exprop op0.x.i8;  break; \
            case PTY_i16: res.x.i16 = exprop op0.x.i16; break; \
            case PTY_i32: res.x.i32 = exprop op0.x.i32; break; \
            case PTY_i64: res.x.i64 = exprop op0.x.i64; break; \
            case PTY_u8:  res.x.u8  = exprop op0.x.u8;  break; \
            case PTY_u16: res.x.u16 = exprop op0.x.u16; break; \
            case PTY_u32: res.x.u32 = exprop op0.x.u32; break; \
            case PTY_u64: res.x.u64 = exprop op0.x.u64; break; \
            default: MIR_FATAL("Unsupported PrimType %d for unary operator %s", exprptyp, #exprop); \
        } \
        res.ptyp = exprptyp; \
    } while(0)

#endif // MPLENG_MEXPRESSION_H_
