#include <stdio.h>
#include <stdlib.h>

unsigned char u8 = 0x80;
unsigned short u16 = 0x8000;
unsigned int u32 = 0x80000000;
unsigned long long u64 = 0x8000000000000000ULL;

signed char i8 = 0x80;
signed short i16 = 0x8000;
signed int i32 = 0x80000000;
signed long long i64 = 0x8000000000000000LL;

int main() {
  signed char ti8 = u8;
  if (ti8 != 0xffffff80) abort();
  signed short ti16 = u8;
  if (ti16 != 0x80) abort();
  signed int ti32 = u8;
  if (ti32 != 0x80) abort();
  signed long long ti64 = u8;
  if (ti64 != 0x80) abort();

  ti8 = u16;
  if (ti8 != 0) abort();
  ti16 = u16;
  if (ti16 != 0xffff8000) abort();
  ti32 = u16;
  if (ti32 != 0x8000) abort();
  ti64 = u16;
  if (ti64 != 0x8000) abort();

  ti8 = u32;
  if (ti8 != 0) abort();
  ti16 = u32;
  if (ti16 != 0) abort();
  ti32 = u32;
  if (ti32 != 0x80000000) abort();
  ti64 = u32;
  if (ti64 != 0x80000000) abort();

  ti8 = u64;
  if (ti8 != 0) abort();
  ti16 = u64;
  if (ti16 != 0) abort();
  ti32 = u64;
  if (ti32 != 0) abort();
  ti64 = u64;
  if (ti64 != 0x8000000000000000) abort();

  unsigned char tu8 = i8;
  if (tu8 != 0x80) abort();
  unsigned short tu16 = i8;
  if (tu16 != 0xff80) abort();
  unsigned int tu32 = i8;
  if (tu32 != 0xffffff80) abort();
  unsigned long long tu64 = i8;
  if (tu64 != 0xffffffffffffff80) abort();

  tu8 = i16;
  if (tu8 != 0) abort();
  tu16 = i16;
  if (tu16 != 0x8000) abort();
  tu32 = i16;
  if (tu32 != 0xffff8000) abort();
  tu64 = i16;
  if (tu64 != 0xffffffffffff8000) abort();

  tu8 = i32;
  if (tu8 != 0) abort();
  tu16 = i32;
  if (tu16 != 0) abort();
  tu32 = i32;
  if (tu32 != 0x80000000) abort();
  tu64 = i32;
  if (tu64 != 0xffffffff80000000) abort();

  tu8 = i64;
  if (tu8 != 0) abort();
  tu16 = i64;
  if (tu16 != 0) abort();
  tu32 = i64;
  if (tu32 != 0) abort();
  tu64 = i64;
  if (tu64 != 0x8000000000000000) abort();
}
