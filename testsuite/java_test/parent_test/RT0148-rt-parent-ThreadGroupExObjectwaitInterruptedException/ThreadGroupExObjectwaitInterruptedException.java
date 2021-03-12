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


import java.lang.ThreadGroup;
public class ThreadGroupExObjectwaitInterruptedException {
    static int res = 99;
    private static ThreadGroup gr1 = new ThreadGroup("Thread8023");
    public static void main(String argv[]) {
        System.out.println(new ThreadGroupExObjectwaitInterruptedException().run());
    }
    private class ThreadGroupExObjectwaitInterruptedException11 implements Runnable {
        // final void wait()
        private int remainder;
        private ThreadGroupExObjectwaitInterruptedException11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (gr1) {
                gr1.notifyAll();
                try {
                    gr1.wait();
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 10;
                }
            }
        }
    }
    private class ThreadGroupExObjectwaitInterruptedException21 implements Runnable {
        // final void wait(long millis)
        private int remainder;
        long millis = 10000;
        private ThreadGroupExObjectwaitInterruptedException21(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (gr1) {
                gr1.notifyAll();
                try {
                    gr1.wait(millis);
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 5;
                }
            }
        }
    }
    private class ThreadGroupExObjectwaitInterruptedException31 implements Runnable {
        // final void wait(long millis, int nanos)
        private int remainder;
        long millis = 10000;
        int nanos = 100;
        private ThreadGroupExObjectwaitInterruptedException31(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (gr1) {
                gr1.notifyAll();
                try {
                    gr1.wait(millis, nanos);
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ThreadGroupExObjectwaitInterruptedException.res = ThreadGroupExObjectwaitInterruptedException.res - 5;
                }
            }
        }
    }
    /**
     * sleep fun
     *
     * @param slpnum wait time
    */

    public void sleep(int slpnum) {
        try {
            Thread.sleep(slpnum);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        // check api normal
        // final void wait()
        Thread t1 = new Thread(new ThreadGroupExObjectwaitInterruptedException11(1));
        // final void wait(long millis)
        Thread t3 = new Thread(new ThreadGroupExObjectwaitInterruptedException21(3));
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ThreadGroupExObjectwaitInterruptedException31(5));
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t3.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t5.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t1.start();
        sleep(1000);
        t1.interrupt();
        sleep(1000);
        t3.start();
        sleep(1000);
        t3.interrupt();
        sleep(1000);
        t5.start();
        sleep(1000);
        t5.interrupt();
        sleep(1000);
        if (result == 2 && ThreadGroupExObjectwaitInterruptedException.res == 96) {
            result = 0;
        }
        return result;
    }
}