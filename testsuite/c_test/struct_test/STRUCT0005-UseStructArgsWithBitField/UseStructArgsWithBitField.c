/* test callee's struct(aggregate) args is the struct args from this function */
#include <stdio.h>
#include <string.h>

typedef struct ArtChildMeta {
  char slotType : 3;  // value, child addr, lpm addr
  char maskLen : 4;   // ip mask len
  char delFlag : 1;   // delete flag for child value slot
} __attribute__((packed)) ArtChildMetaT;

typedef struct ArtRunChildSlot {
  ArtChildMetaT childMeta;
  unsigned long long y;
} ArtRunChildSlotT;

typedef struct ArtChildFloatMeta {
  float c;
} __attribute__((packed)) ArtChildFloatMetaT;

typedef struct ArtRunFloatSlot {
  float a;
  ArtChildFloatMetaT childMeta;
} ArtRunFloatSlotT;

typedef struct ArtRunChildBigSlot {
  ArtChildMetaT childMeta;
  unsigned long long y;
  unsigned long long z;
} ArtRunChildBigSlotT;


__attribute__((noinline)) void bar(ArtChildMetaT data) {
  printf("%d\n", (int)data.slotType);
}

/* aarch64 - 8 register for passing parameters*/
__attribute__((noinline)) void foo(ArtRunChildSlotT childSlot,
    int reg0, int reg1, int reg2, int reg3, int reg4, int reg5, int reg6, int reg7,
    ArtRunChildSlotT childSlotStk)
{
  /* test1: 被调用者的agg（<=16byte直接操作寄存器）入参是当前函数的寄存器agg传参 */
  bar(childSlot.childMeta);
  /* test2：被调用者的agg（<=16byte直接操作寄存器）入参是当前函数的栈agg传参 */
  bar(childSlotStk.childMeta);
}

__attribute__((noinline)) void bar_float(ArtChildFloatMetaT f_data) {
  printf("%.6f\n", f_data.c);
}

__attribute__((noinline)) void foo_float (ArtRunFloatSlotT fChildSlot,
    float s0, float s1, float s2, float s3, float s4, float s5, float s6, float s7,
    ArtRunFloatSlotT fChildSlotStk)
{
  /* test3: 被调用者的float_agg入参是当前函数的寄存器float_agg传参 */
  bar_float(fChildSlot.childMeta);
  /* test4：被调用者的agg入参是当前函数的栈agg传参 */
  bar_float(fChildSlotStk.childMeta);
}

__attribute__((noinline)) void foo_big(ArtRunChildBigSlotT bChildSlot,
    int reg0, int reg1, int reg2, int reg3, int reg4, int reg5, int reg6, int reg7,
    ArtRunChildBigSlotT bChildSlotStk)
{
  /* test5：被调用者的agg(16byte传地址间接操作)入参是当前函数的寄存器agg传参 */
  bar(bChildSlot.childMeta);
  /* test6：被调用者的agg(16byte传地址间接操作)入参是当前函数的栈agg传参 */
  bar(bChildSlotStk.childMeta);
}

ArtRunChildSlotT obj;
ArtRunFloatSlotT objFloat;
ArtRunChildBigSlotT objBig;

int main() {
  memset(&obj, 0, sizeof(obj));
  memset(&objFloat, 0, sizeof(objFloat));
  memset(&objBig, 0, sizeof(objBig));
  foo(obj, 1, 2, 3 ,4 ,5 ,6 ,7, 8, obj);
  foo_float(objFloat, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, objFloat);
  foo_big(objBig, 1, 2, 3 ,4 ,5 ,6 ,7, 8, objBig);
}
