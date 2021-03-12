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
class ThreadCaseClassA {
    private static int field = 1;
    static {
        field = ThreadCaseClassB.getField();
        field++;
    }
    public static int getField() {
        return field;
    }
}
class ThreadCaseClassB {
    private static int field = 2;
    static {
        field = ThreadCaseClassA.getField();
        field++;
    }
    public static int getField() {
        return field;
    }
}
public class ClinitThread003 {
    private static final int THREAD_COUNT = 4096;
    private int result = 0;
    /**
     * CyclicBarrier 适用再多线程相互等待，直到到达一个屏障点。并且CyclicBarrier是可重用的。
    */

    private CyclicBarrier cyclicBarrier = new CyclicBarrier(THREAD_COUNT);
    public static void main(String[] args) {
        ClinitThread003 test = new ClinitThread003();
        ExecutorService executorService = test.runThread();
        executorService.shutdown();
        try {
            if (executorService.awaitTermination(10, TimeUnit.SECONDS)) {
                if (ThreadCaseClassA.getField() != 3 || ThreadCaseClassB.getField() != 2) {
                    System.out.println(1/*STATUS_TEMP*/);
                } else {
                    System.out.println(0/*STATUS_TEMP*/);
                }
            }
        } catch (InterruptedException ex) {
            ex.printStackTrace();
            System.out.println(1/*STATUS_TEMP*/);
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
                    if (ThreadCaseClassA.getField() != 3) {
                        result = ThreadCaseClassB.getField();
                    }
                } catch (InterruptedException | BrokenBarrierException e) {
                    e.printStackTrace();
                }
            }
        });
    }
}
