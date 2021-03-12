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


import java.util.ArrayList;
import java.util.List;
public class StartOOMTest {
    public static void main(String[] args) throws Throwable {
        Runnable r = new SleepRunnable();
        ThreadGroup tg = new ThreadGroup("buggy");
        List<Thread> threads = new ArrayList<Thread>();
        Thread failedThread = null;
        int i = 0;
        for (i = 0; i < 101; i++) {
            Thread t = new Thread(tg, r);
            try {
                if (i == 100) {
                    throw new OutOfMemoryError("throw OutOfMemoryError");
                }
                t.start();
                threads.add(t);
            } catch (Throwable x) {
                failedThread = t;
                System.out.println(x);
                System.out.println(i);
                break;
            }
        }
        int j = 0;
        for (Thread t : threads) {
            t.interrupt();
        }
        while (tg.activeCount() > i / 2) {
            Thread.yield();
        }
        failedThread.start();
        failedThread.interrupt();
        for (Thread t : threads) {
            t.join();
        }
        failedThread.join();
        try {
            Thread.sleep(1000);
        } catch (Throwable ignore) {
            System.out.println("Sleep is interrupted");
        }
        int activeCount = tg.activeCount();
        System.out.println("activeCount = " + activeCount);
        if (activeCount > 0) {
            throw new RuntimeException("Failed: there  should be no active Threads in the group");
        }
    }
    static class SleepRunnable implements Runnable {
        public void run() {
            try {
                Thread.sleep(60 * 1000);
            } catch (Throwable t) {
            }
        }
    }
}