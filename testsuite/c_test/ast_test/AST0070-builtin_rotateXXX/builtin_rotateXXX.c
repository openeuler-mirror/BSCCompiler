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
#include <stdio.h>
#include <math.h>
#include <stdint.h>

int ConvertBinaryToDecimal(long long n) {
  int dec = 0, i = 0, rem;
  while (n != 0) {
    rem = n % 10;
    n /= 10;
    dec += rem * pow(2, i);
    ++i;
  }
  return dec;
}

int main() {
  unsigned int x = ConvertBinaryToDecimal(10000110);
  unsigned int z = __builtin_rotateleft8(x, 11);
  printf("%u\n", z);  // 52: 0011 0100
  x = ConvertBinaryToDecimal(1000001101000010);
  z = __builtin_rotateleft16(x, 23);
  printf("%u\n", z);  // 41281: 1010 0001 0100 0001
  x = ConvertBinaryToDecimal(1000001101000010);
  z = __builtin_rotateleft32(x, 23);
  printf("%u\n", z);  // 2701131841: 1010 0001 0000 0000 0000 0000 0100 0001
  uint64_t w = __builtin_rotateleft64(3, 63);
  printf("%llu\n", w);  // 9223372036854775809: 1000000000000000000000000000000000000000000000000000000000000001
  printf("\n");

  x = ConvertBinaryToDecimal(10000110);
  z = __builtin_rotateright8(x, 11);
  printf("%u\n", z);  // 208: 1101 0000
  x = ConvertBinaryToDecimal(1000001101000010);
  z = __builtin_rotateright16(x, 23);
  printf("%u\n", z);  // 34054: 1000 0101 000 00110
  x = ConvertBinaryToDecimal(1000001101000010);
  z = __builtin_rotateright32(x, 23);
  printf("%u\n", z);  // 17204224: 0000 0001 0000 0110 1000 0100 0000 0000
  w = __builtin_rotateright64(3, 1);
  printf("%llu\n", w);  // 9223372036854775809: 1000000000000000000000000000000000000000000000000000000000000001

  return 0;
}
