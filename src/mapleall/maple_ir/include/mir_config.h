/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
// configuration definition for code in maple_ir namespace
#ifndef MAPLE_IR_INCLUDE_MIR_CONFIG_H
#define MAPLE_IR_INCLUDE_MIR_CONFIG_H

// MIR_FEATURE_FULL = 1 : for host/server size building, by default.
// MIR_FEATURE_FULL = 0 : for resource-constrained devices. optimized for memory size
#if !defined(MIR_FEATURE_FULL)
#define MIR_FEATURE_FULL 1  // default to full feature building, for debugging
#endif                      // MIR_FEATURE_FULL define

// MIR_DEBUG = 0 : for release building.
// MIR_DEBUG = 1 : for debug building.
#ifndef MIR_DEBUG
#define MIR_DEBUG 0  // currently default to none. turn it on explicitly
#endif               // MIR_DEBUG

// MIR_DEBUG_LEVEL = 0: no debuging information at all.
//                   1: with error information.
//                   2: with severe warning information
//                   3: with normal warning information
//                   4: with normal information
//                   5: with everything
//
#ifndef MIR_DEBUG_LEVEL
#define MIR_DEBUG_LEVEL 0
#endif  // MIR_DEBUG_LEVEL
// assertion
#if !MIR_FEATURE_FULL
#define MIR_ASSERT(...) \
  do {                  \
  } while (0)
#define MIR_PRINTF(...) \
  do {                  \
  } while (0)
#define MIR_INFO(...) \
  do {                \
  } while (0)
#define MIR_ERROR(...) \
  do {                 \
  } while (0)
#define MIR_WARNING(...) \
  do {                   \
  } while (0)
#define MIR_CAST_TO(var, totype) ((totype)(var))
#include <stdlib.h>
#if DEBUG
#include <stdio.h>
#define MIR_FATAL(...)                                   \
  do {                                                   \
    printf("FATAL ERROR: (%s:%d) ", __FILE__, __LINE__); \
    printf(__VA_ARGS__);                                 \
    exit(1);                                             \
  } while (0)
#else
#define MIR_FATAL(...) \
  do {                 \
    exit(1);           \
  } while (0)
#endif  // DEBUG
#else   // MIR_FEATURE_FULL
#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace maple {
#define MIR_ASSERT(...) assert(__VA_ARGS__)
#define MIR_FATAL(...)                                            \
  do {                                                            \
    fprintf(stderr, "FATAL ERROR: (%s:%d) ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                                 \
    exit(EXIT_FAILURE);                                           \
  } while (0)
#define MIR_ERROR(...)                                      \
  do {                                                      \
    fprintf(stderr, "ERROR: (%s:%d) ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                           \
  } while (0)
#define MIR_WARNING(...)                                      \
  do {                                                        \
    fprintf(stderr, "WARNING: (%s:%d) ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                             \
  } while (0)
#define MIR_PRINTF(...) printf(__VA_ARGS__)
#define MIR_INFO(...) printf(__VA_ARGS__)
#define MIR_CAST_TO(var, totype) static_cast<totype>(var)
#endif  // !MIR_FEATURE_FULL
#if MIR_DEBUG
#else
#endif  // MIR_DEBUG

// MIR specific configurations.
// Note: fix size definition cannot handle arbitary long MIR lines, such
// as those array initialization lines.
constexpr int kMirMaxLineSize = 3072;  // a max of 3K characters per line initially
// LIBRARY API availability
#if MIR_FEATURE_FULL
#define HAVE_STRTOD 1  // strtod
#define HAVE_MALLOC 1  // malloc/free
#else                  // compact VM
#define HAVE_STRTOD 1  // strtod in current libc
#define HAVE_MALLOC 0  // no malloc/free in current libc
#endif                 // MIR_FEATURE_FULL
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_CONFIG_H
