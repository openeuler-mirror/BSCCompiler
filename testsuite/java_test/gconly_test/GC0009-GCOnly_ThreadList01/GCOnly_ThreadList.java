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
import java.lang.Thread;
public class GCOnly_ThreadList {
  public synchronized void DebugFunc() {
    for (int i = 0; i < 10000; ++i) {
      int tid = java.lang.Thread.getLockOwnerThreadId(this);
    }
  }
  public static void main(String[] args) {
    try {
      GCOnly_ThreadList obj = new GCOnly_ThreadList();
      SyncThread[] syncThread = new SyncThread[10];
      for (int i = 0; i < 10; i++) {
        syncThread[i] = new SyncThread(obj);
      }
      GCTriggerThread gcThread = new GCTriggerThread();
      for (int i = 0; i < 10; ++i) {
        syncThread[i].start();
      }
      gcThread.start();
      for (int i = 0; i < 10; ++i) {
        syncThread[i].join();
      }
      gcThread.join();
    } catch (Exception e) {
      e.printStackTrace();
    }
  }
}
class SyncThread extends Thread {
  private GCOnly_ThreadList obj = null;
  SyncThread(GCOnly_ThreadList mobj) {
    obj = mobj;
  }
  public void run() {
    obj.DebugFunc();
  }
}

class GCTriggerThread extends Thread {
  public void run() {
    for (int i = 0; i < 1000; ++i) {
      Object[] matrix = new Object[1000000];
      Runtime.getRuntime().gc();
    }
  }
}
