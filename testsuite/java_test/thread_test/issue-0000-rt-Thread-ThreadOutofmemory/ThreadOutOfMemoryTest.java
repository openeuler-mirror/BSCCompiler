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


public class ThreadOutOfMemoryTest {
    public static final int NTHREADS = 5;
    public static final int TIMEOUT = 10;
    public static final int LEVEL1_SIZE = 1048576;
    public static final int LEVEL2_INIT_SIZE = 1048576;
    public static void memoryBuster() {
        int[][] root = new int[LEVEL1_SIZE][];
        int size = LEVEL2_INIT_SIZE; // 1M
        // If we repeat this, it may consume up to 1M * 2G * 4B = 8PB of memory
        for (int i = 0; i < LEVEL1_SIZE; i++) {
            int[] level2 = new int[size];
            root[i] = level2;
            if (size < Integer.MAX_VALUE / 2) {
                size = size * 2;
            } else {
                size = Integer.MAX_VALUE;
            }
        }
    }
    public static void main(String[] args) throws Exception {
        Thread[] childThreads = new Thread[NTHREADS];
        final boolean[] caught = new boolean[NTHREADS];
        for (int i = 0; i < NTHREADS; i++) {
            final int threadIndex = i;
            childThreads[threadIndex] = new Thread(() -> {
                try {
                    memoryBuster();
                } catch (OutOfMemoryError e) {
                    caught[threadIndex] = true;
                }
            }, "Thread-" + threadIndex);
        }
        for (Thread thread : childThreads) {
            thread.start();
        }
        for (Thread thread : childThreads) {
            thread.join();
        }
        for (int i = 0; i < NTHREADS; i++) {
            if (!caught[i]) {
                System.out.printf("ERROR: Thread %d did not catch OutOfMemoryException.%n", i);
            }
        }
        System.out.println("DONE");
    }
}
