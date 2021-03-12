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


import java.lang.ref.WeakReference;
import java.util.ArrayList;
class MyThread extends Thread {
    public void run(){
        try {
            Thread.sleep(index * FrequentSystemGC_01.THREAD_SLEEP_UNIT);
        } catch (Exception e) {
            // System.out.println("Exception from thread sleep: " + index);
            // do nothing, just continue test...
        }
        WeakReference weak = new WeakReference<Object>(new Object());
        if (weak.get() == null) {
            return;
        }
        Runtime.getRuntime().gc();
        if (weak.get() != null) {
            System.out.println("MyThread" + index + " check fail");
            FrequentSystemGC_01.totalResult = false;
        }
   }
   public MyThread(int i) {
       index = i;
   }
   private int index;
}
public class FrequentSystemGC_01 {
    public static final int THREAD_COUNT = 50;
    public static boolean totalResult = true;
    public static final long THREAD_SLEEP_UNIT = 1;
    public static ArrayList<Thread> threads = new ArrayList<>();
    public static void main(String[] args) {
        for (int i = 0; i < THREAD_COUNT; i++) {
            Thread t = new MyThread(i);
            threads.add(t);
            t.start();
        }
        for (int i = 0; i < THREAD_COUNT; i++) {
            try {
                threads.get(i).join();
            } catch (Exception e) {
                System.out.println("Exception from join thread: " + i + " need re-test!");
                return;
            }
        }
        if (FrequentSystemGC_01.totalResult == true) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, weak is not freed after system.gc() in some thread!");
        }
    }
}
