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


import java.util.concurrent.atomic.AtomicInteger;
class ThreadId {
    private static final AtomicInteger nextId = new AtomicInteger(0);
    private static final ThreadLocal<Integer> threadId = new ThreadLocal<Integer>() {
                @Override
                protected Integer initialValue() {
                    return nextId.getAndIncrement();
                }
            };
    public static int get() {
        return threadId.get();
    }
}
public final class TestThreadId extends Thread {
    private static final int ITERATIONCOUNT = 50;
    private static final int THREADCOUNT = 50;
    private static ThreadId id = new ThreadId();
    private int value;
    public static void main(String[] args) throws Throwable {
        boolean[] check = new boolean[THREADCOUNT * ITERATIONCOUNT];
        TestThreadId[] u = new TestThreadId[THREADCOUNT];
        for (int i = 0; i < ITERATIONCOUNT; i++) {
            for (int t = 0; t < THREADCOUNT; t++) {
                u[t] = new TestThreadId();
                u[t].start();
            }
            for (int t = 0; t < THREADCOUNT; t++) {
                try {
                    u[t].join();
                } catch (InterruptedException e) {
                    throw new RuntimeException(
                            "TestThreadId: Failed with unexpected exception" + e);
                }
                try {
                    if (check[u[t].getIdValue()]) {
                        throw new RuntimeException(
                                "TestThreadId: Failed with duplicated id: " +
                                        u[t].getIdValue());
                    } else {
                        check[u[t].getIdValue()] = true;
                    }
                } catch (Exception e) {
                    throw new RuntimeException(
                            "TestThreadId: Failed with unexpected id value" + e);
                }
            }
        }
        System.out.println("0");
    }
    private synchronized int getIdValue() {
        return value;
    }
    public void run() {
        value = ThreadId.get();
    }
}