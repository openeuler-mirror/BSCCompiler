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


public class ThreadTest {
    public static void main(String[] args) throws Exception {
        /**
         * Start the already started thread
        */

        ThreadRunning thread_obj1 = new ThreadRunning();
        thread_obj1.start();
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            throw new RuntimeException("InterruptedException from sleep");
        }
        System.out.println("Thread 1' state -- " + thread_obj1.getState());
        try {
            thread_obj1.start();
            System.out.println("IllegalThreadStateException is expected when starting a started thread");
        } catch (IllegalThreadStateException e) {
            System.out.println("IllegalThreadStateException has been thrown when start the already started thread");
        }
        /**
         * Start the already finished thread
        */

        ThreadRunning thread_obj2 = new ThreadRunning();
        thread_obj2.start();
        try {
            thread_obj2.join();
        } catch (InterruptedException e) {
            System.out.println("INTERRUPTED_MESSAGE");
        }
        System.out.println("Thread 2' state -- " + thread_obj2.getState());
        try {
            thread_obj2.start();
            System.out.println("IllegalThreadStateException is expected when starting a finished thread");
        } catch (IllegalThreadStateException e) {
            System.out.println("IllegalThreadStateException has been thrown when start the already finished thread");
        }
        System.out.println("PASS");
    }
    // test for start()
    static class ThreadRunning extends Thread {
        ThreadRunning() {
            super();
        }
        public void run() {
            try {
                Thread.sleep(3000);
            } catch (InterruptedException e) {
                throw new RuntimeException("InterruptedException from sleep");
            }
        }
    }
}