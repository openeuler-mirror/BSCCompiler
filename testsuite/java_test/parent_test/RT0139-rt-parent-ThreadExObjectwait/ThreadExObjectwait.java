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


import java.lang.Thread;
public class ThreadExObjectwait {
    static int res = 99;
    Thread sb = new Thread();
    public static void main(String argv[]) {
        System.out.println(new ThreadExObjectwait().run());
    }
    private class ThreadExObjectwait11 implements Runnable {
        // final void wait()
        private int remainder;
        private ThreadExObjectwait11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (sb) {
                sb.notifyAll();
                try {
                    sb.wait();
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 10;
                }
            }
        }
    }
    private class ThreadExObjectwait21 implements Runnable {
        // final void wait(long millis)
        private int remainder;
        long millis = 10;
        private ThreadExObjectwait21(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (sb) {
                sb.notify();
                try {
                    sb.wait(millis);
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 10;
                } catch (IllegalArgumentException e3) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 5;
                }
            }
        }
    }
    private class ThreadExObjectwait31 implements Runnable {
        // final void wait(long millis, int nanos)
        private int remainder;
        long millis = 10;
        int nanos = 10;
        private ThreadExObjectwait31(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (sb) {
                sb.notifyAll();
                try {
                    sb.wait(millis, nanos);
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 10;
                } catch (IllegalArgumentException e3) {
                    ThreadExObjectwait.res = ThreadExObjectwait.res - 5;
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
    private void waitFinish(Thread thread) {
        try {
            thread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    private void sbSetUncaught() {
        sb.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
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
        Thread t1 = new Thread(new ThreadExObjectwait11(1));
        Thread t2 = new Thread(new ThreadExObjectwait11(2));
        // final void wait(long millis)
        Thread t3 = new Thread(new ThreadExObjectwait21(3));
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ThreadExObjectwait31(5));
        sbSetUncaught();
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t2.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
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
        t3.start();
        sleep(1000);
        t5.start();
        sleep(1000);
        t2.start();
        t1.interrupt();
        waitFinish(t1);
        waitFinish(t3);
        waitFinish(t5);
        if (result == 2 && ThreadExObjectwait.res == 54) {
            result = 0;
        }
        t2.interrupt();
        return result;
    }
}