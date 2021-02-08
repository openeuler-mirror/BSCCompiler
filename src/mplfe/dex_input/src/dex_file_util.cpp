/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "dex_file_util.h"
namespace maple {
namespace bc {
const uint8 DexFileUtil::kDexFileMagic[] = {0x64, 0x65, 0x78, 0x0a, 0x30, 0x33, 0x39, 0x00}; // dex\n039\0
const uint8 DexFileUtil::kEndianConstant[] = {0x12, 0x34, 0x56, 0x78}; // 0x12345678
const uint8 DexFileUtil::kReverseEndianConstant[] = {0x78, 0x56, 0x34, 0x12}; // 0x78563412

uint32 DexFileUtil::Adler32(const uint8 *data, uint32 size) {
  // May be different with adler32 in libz
  uint32 kMod = 65521;
  // Calculate A
  uint16 a = 1;
  for (uint32 i = 0; i < size; ++i) {
    a += *(data + i); // truncate the data over 16bit
  }
  a %= kMod;

  // Caculate B
  uint16 item = 1;
  uint16 b = 0;
  for (uint32 i = 0; i < size; ++i) {
    item = item + *(data + i);
    b += item;
  }
  b %= kMod;

  return (static_cast<uint32>(b) << 16) | static_cast<uint32>(a);
}
}  // namespace bc
}  // namespace maple
