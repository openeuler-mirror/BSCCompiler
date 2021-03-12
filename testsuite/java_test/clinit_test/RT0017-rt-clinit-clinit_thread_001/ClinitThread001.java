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
class CaseThreadClass {
    static CaseThreadClass member;
    static int field;
    static {
        if (ClinitThread001.getCount() == 0) {
            // 不会重复触发初始化
            member = new CaseThreadClass();
            field = ClinitThread001.getCount();
        }
    }
    CaseThreadClass() {
        ClinitThread001.getCount();
    }
}
class ClassA {
    static ClassB member;
    static int field;
    static {
        if (ClinitThread001.getCount() == 3) {
            field = ClinitThread001.getCount();
            member = new ClassB();
        }
    }
    ClassA() {
        ClinitThread001.getCount();
    }
}
class ClassB {
    static ClassA member;
    static int field;
    static {
        if (ClinitThread001.getCount() == 5) {
            member = new ClassA();
            field = ClinitThread001.getCount();
        }
    }
    ClassB() {
        ClinitThread001.getCount();
    }
}
public class ClinitThread001 {
    private static final int THREAD_COUNT = 10;
    private static int count = 0;
    // CyclicBarrier 适用再多线程相互等待，直到到达一个屏障点。并且CyclicBarrier是可重用的
    private CyclicBarrier cyclicBarrier = new CyclicBarrier(THREAD_COUNT);
    static int getCount() {
        return count++;
    }
    public static void main(String[] args) {
        ClinitThread001 test = new ClinitThread001();
        ExecutorService executorService = test.runThread();
        executorService.shutdown();
        try {
            if (executorService.awaitTermination(10, TimeUnit.SECONDS)) {
                if (count != 9) {
                    System.out.println(1);
                } else {
                    System.out.println(0);
                }
            }
        } catch (InterruptedException ex) {
            ex.printStackTrace();
            System.out.println(1);
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
                    if (CaseThreadClass.field != 2) {
                        System.out.println("Error, CaseThreadClass not initialized");
                    }
                    if (ClassA.field != 4) {
                        System.out.println("Error, ClassA not initialized");
                    }
                    if (ClassB.field != 7) {
                        System.out.println("Error, ClassB not initialized");
                    }
                } catch (InterruptedException | BrokenBarrierException e) {
                    e.printStackTrace();
                }
            }
        });
    }
}
