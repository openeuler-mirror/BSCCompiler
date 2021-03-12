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
public class WeakRefTest {
  static Reference rp;
  static ReferenceQueue rq = new ReferenceQueue();
  static int a = 100;
  static void setWeakReference() {
    rp = new WeakReference<Object>(new Object(), rq);
    if (rp.get() == null) {
      a++;
    }
  }
  static class TriggerRP implements Runnable {
    public void run() {
      for (int i = 0; i < 60; i++) {
        WeakReference wr = new WeakReference(new Object());
        try {
          Thread.sleep(50);
        } catch (Exception e) {}
      }
    }
  }
  public static void main(String [] args) throws Exception {
    Reference r;
    setWeakReference();
    new Thread(new TriggerRP()).start();
    Thread.sleep(3000);
    if (rp.get() != null) {
      a++;
    }
    while ((r = rq.poll())!=null) {
      if (!r.getClass().toString().equals("class java.lang.ref.WeakReference")) {
        a++;
      }
    }
    if (a == 100) {
      System.out.println("ExpectResult");
    }
  }
}
