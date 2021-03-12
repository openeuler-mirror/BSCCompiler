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


import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
class ThreadClass {
    static volatile int field;
    static {
        try {
            int temp = ClinitEh003.getCount();
            if (temp == 0) {
                int a = ClinitEh003.getCount();
                int b = ClinitEh003.getCount();
                field = a / (b - 2);
            }
        } catch (Exception e) {
            ClinitEh003.getCount();
        }
    }
}
public class ClinitEh003 {
    private static final int THREAD_COUNT = 10;
    private static int count = 0;
    static volatile private int initErrCount = 0;
    static volatile private int notDefCount = 0;
    // CyclicBarrier 适用再多线程相互等待，直到到达一个屏障点。并且CyclicBarrier是可重用的
    private CyclicBarrier cyclicBarrier = new CyclicBarrier(THREAD_COUNT);
    static int getCount() {
        return count++;
    }
    static synchronized void InitializerErrorCnt() {
        initErrCount += 1;
    }
    static synchronized void NotDefErrorCnt() {
        notDefCount += 1;
    }
    public static void main(String[] args) {
        ClinitEh003 test = new ClinitEh003();
        ExecutorService executorService = test.runThread();
        executorService.shutdown();
        try {
            if (executorService.awaitTermination(10, TimeUnit.SECONDS)) {
                if (count != 4) {
                    System.out.println(1);
                } else {
                    System.out.println(0);
                }
            }
        } catch (InterruptedException ex) {
            ex.printStackTrace();
        }
        if (initErrCount != 0) {
            System.out.println("Should be only invoked once, but actual invoked "
                    + initErrCount);
        }
        if (notDefCount != 0) {
            System.out.println("Should be invoked " + (THREAD_COUNT - 1)
                    + ", but actual invoked " + notDefCount);
        }
    }
    private ExecutorService runThread() {
        ExecutorService executorService = Executors.newFixedThreadPool(THREAD_COUNT);
        for (int i = 0; i < THREAD_COUNT; i++) {
            executorService.submit(createThread(i));
        }
        return executorService;
    }
    private Thread createThread(int i) {
        return new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    cyclicBarrier.await();
                    try {
                        if (ThreadClass.field == 0) {
                            ThreadClass.field = 5;
                        }
                    } catch (ExceptionInInitializerError e) {
                        ClinitEh003.InitializerErrorCnt();
                    } catch (NoClassDefFoundError e) {
                        ClinitEh003.NotDefErrorCnt();
                    }
                } catch (InterruptedException | BrokenBarrierException e) {
                    e.printStackTrace();
                }
            }
        });
    }
}
