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


public class Basic {
    static ThreadLocal n = new ThreadLocal() {
        int i = 0;
        protected synchronized Object initialValue() {
            return new Integer(i++);
        }
    };
    public static void main(String[] args) throws Exception {
        int threadCount = 100;
        Thread[] th = new Thread[threadCount];
        final int[] x = new int[threadCount];
        // Start the threads
        for (int i = 0; i < threadCount; i++) {
            th[i] = new Thread() {
                public void run() {
                    int threadId = ((Integer) (n.get())).intValue();
                    for (int j = 0; j < threadId; j++) {
                        x[threadId]++;
                        yield();
                    }
                }
            };
            th[i].start();
        }
        // Wait for the threads to finish
        for (int i = 0; i < threadCount; i++)
            th[i].join();
        // Check results
        for (int i = 0; i < threadCount; i++) {
            if (x[i] != i)
                throw (new Exception("x[" + i + "] =" + x[i]));
            System.out.println(x[i]);
        }
    }
}