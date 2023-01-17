// CHECK: [[# FILENUM:]] "{{.*}}/rettype.c"
#include <stdio.h>
#include <string.h>
#include <limits.h>

static unsigned short AI[16];
static unsigned short init_qi[16] = { 3,5,7,9,0,0,0,0,0,0,0,0,0,0,0,0 };
static unsigned short BI[16] = { 2,4,6,8,1,1,1,1,1,1,1,1,1,1,1,1};
static unsigned short test_qi[16] = { 3,5,7,9,0,0,1,65535,0,1,1,1,65535,0,1,1 };

int do_qi ()
{
  __sync_lock_release(BI + 4);
  if (BI[4] != 0) {
    printf("BI[4]=%u\n",BI[4]);
  }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_fetch_and_add_2
  if (__sync_fetch_and_add(AI+6, BI[6]) != 0)
    printf("AI[6]=%u\n",AI[6]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_fetch_and_sub_2
  if (__sync_fetch_and_sub(AI+7, BI[7]) != 0)
    printf("AI[7]=%u\n",AI[7]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_fetch_and_and_2
  if (__sync_fetch_and_and(AI+8, BI[8]) != 0)
    printf("AI[8]=%u\n",AI[8]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_fetch_and_or_2
  if (__sync_fetch_and_or(AI+9, BI[9]) != 0)
    printf("AI[9]=%u\n",AI[9]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_fetch_and_xor_2
  if (__sync_fetch_and_xor(AI+10, BI[10]) != 0)
    printf("AI[10]=%u\n",AI[10]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_add_and_fetch_2
  if (__sync_add_and_fetch(AI+11, BI[11]) != 1)
    printf("AI[11]=%u\n",AI[11]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_sub_and_fetch_2
  if (__sync_sub_and_fetch(AI+12, BI[12]) != 65535)
    {
        printf("AI[12]=%u\n",AI[12]);
        printf("__sync_sub_and_fetch(AI+12, BI[12])=%u\n",__sync_sub_and_fetch(AI+12, BI[12]));
    }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_and_and_fetch_2
  if (__sync_and_and_fetch(AI+13, BI[13]) != 0)
    printf("AI[13]=%u\n",AI[13]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_or_and_fetch_2
  if (__sync_or_and_fetch(AI+14, BI[14]) != 1)
    printf("AI[14]=%u\n",AI[14]);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned u16 C___sync_xor_and_fetch_2
  if (__sync_xor_and_fetch(AI+15, BI[15]) != 1)
    printf("AI[15]=%u\n",AI[15]);

    return 0;
}


int main()
{
  memcpy(AI, init_qi, sizeof(init_qi));

  do_qi ();

  if (memcmp (AI, test_qi, sizeof(test_qi)))
    return 1;

  return 0;
}
