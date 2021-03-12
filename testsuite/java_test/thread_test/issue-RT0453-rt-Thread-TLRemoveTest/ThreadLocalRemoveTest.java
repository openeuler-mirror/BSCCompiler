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


public class ThreadLocalRemoveTest {
    private static final int INITIAL_VALUE = 101;
    private static final int REMOVE_SET_VALUE = 102;
    static ThreadLocal<Integer> n = new ThreadLocal<Integer>() {
        protected synchronized Integer initialValue() {
            return INITIAL_VALUE;
        }
    };
    public static void main(String[] args) throws Throwable {
        int threadCount = 100;
        final int[] removeNode = {10, 20, 45, 38};
        // ThreadLocal values will be removed for these threads.
        final int[] removeAndSet = {12, 34, 10};
        // ThreadLocal values will be removed and sets new values..
        Thread[] th = new Thread[threadCount];
        final int[] x = new int[threadCount];
        final Throwable[] exceptions = new Throwable[threadCount];
        for (int i = 0; i < threadCount; i++) {
            final int threadId = i;
            th[i] = new Thread() {
                public void run() {
                    try {
                        n.set(threadId); // Sets threadId as threadLocal value...
                        for (int j = 0; j < threadId; j++) {
                            yield();
                        }
                        // To remove the ThreadLocal ....
                        for (int removeId : removeNode) {
                            if (threadId == removeId) {
                                n.remove(); // Removes ThreadLocal values..
                                break;
                            }
                        }
                        // To remove the ThreadLocal value and set new value ...
                        for (int removeId : removeAndSet) {
                            if (threadId == removeId) {
                                n.remove(); // Removes the ThreadLocal Value...
                                n.set(REMOVE_SET_VALUE); /* Setting new Values to
                                                          ThreadLocal*/

                                break;
                            }
                        }
                        /* Storing the threadLocal values in 'x'
                           ...so that it can be used for checking results...*/

                        x[threadId] = n.get();
                    } catch (Throwable ex) {
                        exceptions[threadId] = ex;
                    }
                }
            };
            th[i].start();
        }
        // Wait for the threads to finish
        for (int i = 0; i < threadCount; i++) {
            th[i].join();
        }
        // Check results
        for (int i = 0; i < threadCount; i++) {
            int checkValue = i;
            /* If the remove method is called then the ThreadLocal value will
             * be its initial value*/

            for (int removeId : removeNode) {
                if (removeId == i) {
                    checkValue = INITIAL_VALUE;
                    break;
                }
            }
            for (int removeId : removeAndSet) {
                if (removeId == i) {
                    checkValue = REMOVE_SET_VALUE;
                    break;
                }
            }
            if (exceptions[i] != null) {
                throw (exceptions[i]);
            }
            if (x[i] != checkValue) {
                throw (new Throwable("x[" + i + "] =" + x[i]));
            }
        }
        System.out.println("0");
    }
}