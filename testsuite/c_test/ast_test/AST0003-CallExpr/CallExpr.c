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

int GetValue(int a) {
  return a;
}
int AddTwo(int a, int b){                     // multi param
  return GetValue(a) + GetValue(b);
}
void AssignArr(int a[], int len){             // array param
  for (int i = 0; i < len; i++){
    a[i] = i * 2;
  }
}
void PrintArr(int a[], int len){
  int i = 0;
  while (i < len){
    printf("%d", a[i]);
    i++;
  }
  printf("\n");
}
int CulSum(int a){                            // recursive call
  if (a == 1){
    return 1;
  }
  else{
    return a + CulSum(a - 1);
  }
}

int main() {
  int b = 23;
  int c = GetValue(b);
  printf("%d", c);

  // add more complex cases
  printf("b + c = %d\n", AddTwo(b, c));
  int arr[5];
  int len = sizeof(arr)/sizeof(int);
  AssignArr(arr, len);
  // PrintArr(arr, len);                      // @fail case, output wrong
  printf("%d", CulSum(10));

  return 0;
}
