/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.lang.ref.*;
public class PhantomRefTest {
  static Reference rp;
  static ReferenceQueue rq = new ReferenceQueue();
  static int a = 100;
  static void setPhantomReference() {
    rp = new PhantomReference<Object>(new Object(), rq);
    if (rp.get() != null) {
      a++;
    }
  }
  public static void main(String [] args) throws Exception {
    Reference r;
    setPhantomReference();
    Thread.sleep(2000);
    while ((r = rq.poll())!=null) {
      if (!r.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
        a++;
      }
    }
    if (a == 100) {
      System.out.println("ExpectResult");
    }
  }
}
