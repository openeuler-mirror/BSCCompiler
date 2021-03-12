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
public class StringExObjectwait {
    static int res = 99;
    private String mf2 = "this is a test";
    static Thread t2;
    public static void main(String argv[]) {
        System.out.println(new StringExObjectwait().run());
        t2.interrupt();
    }
    private class StringExObjectwait11 implements Runnable {
        // final void wait()
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (mf2) {
                mf2.notifyAll();
                try {
                    mf2.wait();
                    StringExObjectwait.res = StringExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    StringExObjectwait.res = StringExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    StringExObjectwait.res = StringExObjectwait.res - 10;
                }
            }
        }
    }
    private class StringExObjectwait21 implements Runnable {
        // final void wait(long millis)
        long millis = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (mf2) {
                mf2.notify();
                try {
                    mf2.wait(millis);
                    StringExObjectwait.res = StringExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    StringExObjectwait.res = StringExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    StringExObjectwait.res = StringExObjectwait.res - 10;
                } catch (IllegalArgumentException e3) {
                    StringExObjectwait.res = StringExObjectwait.res - 5;
                }
            }
        }
    }
    private class StringExObjectwait31 implements Runnable {
        // final void wait(long millis, int nanos)
        long millis = 10;
        int nanos = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (mf2) {
                mf2.notifyAll();
                try {
                    mf2.wait(millis, nanos);
                    StringExObjectwait.res = StringExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    StringExObjectwait.res = StringExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    StringExObjectwait.res = StringExObjectwait.res - 10;
                } catch (IllegalArgumentException e3) {
                    StringExObjectwait.res = StringExObjectwait.res - 5;
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
     * join fun
     * @param thread wait join thread
    */

    private void waitFinish(Thread thread) {
        try {
            thread.join();
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
        Thread t1 = new Thread(new StringExObjectwait11());
        t2 = new Thread(new StringExObjectwait11());
        // final void wait(long millis)
        Thread t3 = new Thread(new StringExObjectwait21());
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new StringExObjectwait31());
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
        t3.start();
        sleep(1000);
        t5.start();
        sleep(1000);
        t2.start();
        t1.interrupt();
        waitFinish(t1);
        waitFinish(t3);
        waitFinish(t5);
        if (result == 2 && StringExObjectwait.res == 54) {
            result = 0;
        }
        t2.interrupt();
        return result;
    }
}