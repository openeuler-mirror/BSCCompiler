//
//Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
//
//OpenArkFE is licensed under the Mulan PSL v1.
//You can use this software according to the terms and conditions of the Mulan PSL v1.
//You may obtain a copy of Mulan PSL v1 at:
//
// http://license.coscl.org.cn/MulanPSL
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
//FIT FOR A PARTICULAR PURPOSE.
//See the Mulan PSL v1 for more details.
//

// Java has a rule 'Definite Assignment'.
// This case is from JLS 8.
//
// In this case, k is not guaranteed to be assigned before println.
class Point {
  final int x;
  int foo() {
    int k;
    while (n < 4) {
      k = n;
      if (k >= 5) break;
      n = 6;
    }
    System.out.println(k);
  }
}
