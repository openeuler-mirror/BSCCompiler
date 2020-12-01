/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -@TestCaseID: ThrowableExObjectwaitInterruptedException.java
 * -@TestCaseName: Exception in Throwable: final void wait(*)
 * -@TestCaseType: Function Test
 * -@RequirementName: 补充重写类的父类方法
 * -@Brief:
 * -#step1:Test Throwable api wait extends from Object
 * -#step2:Thread is interruptted when wait
 * -#step3:Throw InterruptedException
 * -@Expect:0\n
 * -@Priority: High
 * -@Source: ThrowableExObjectwaitInterruptedException.java
 * -@ExecuteClass: ThrowableExObjectwaitInterruptedException
 * -@ExecuteArgs:
 */

import java.lang.Throwable;

public class ThrowableExObjectwaitInterruptedException {
    static int res = 99;
    static Throwable cause = new Throwable("detailed message of cause");

    public static void main(String argv[]) {
        System.out.println(new ThrowableExObjectwaitInterruptedException().run());
    }

    private class ThrowableExObjectwaitInterruptedException11 implements Runnable {
        // final void wait()
        private int remainder;

        private ThrowableExObjectwaitInterruptedException11(int remainder) {
            this.remainder = remainder;
        }

        /**
         * Thread run fun
         */
        public void run() {
            synchronized (cause) {
                cause.notifyAll();
                try {
                    cause.wait();
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 10;
                }
            }
        }
    }

    private class ThrowableExObjectwaitInterruptedException21 implements Runnable {
        // final void wait(long millis)
        private int remainder;
        long millis = 10000;

        private ThrowableExObjectwaitInterruptedException21(int remainder) {
            this.remainder = remainder;
        }

        /**
         * Thread run fun
         */
        public void run() {
            synchronized (cause) {
                cause.notifyAll();
                try {
                    cause.wait(millis);
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 5;
                }
            }
        }
    }

    private class ThrowableExObjectwaitInterruptedException31 implements Runnable {
        // final void wait(long millis, int nanos)
        private int remainder;
        long millis = 10000;
        int nanos = 100;

        private ThrowableExObjectwaitInterruptedException31(int remainder) {
            this.remainder = remainder;
        }

        /**
         * Thread run fun
         */
        public void run() {
            synchronized (cause) {
                cause.notifyAll();
                try {
                    cause.wait(millis, nanos);
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 15;
                } catch (InterruptedException e1) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ThrowableExObjectwaitInterruptedException.res = ThrowableExObjectwaitInterruptedException.res - 5;
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
        Thread t1 = new Thread(new ThrowableExObjectwaitInterruptedException11(1));
        // final void wait(long millis)
        Thread t3 = new Thread(new ThrowableExObjectwaitInterruptedException21(3));
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ThrowableExObjectwaitInterruptedException31(5));
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
        sleep(100);
        t1.interrupt();
        sleep(100);
        t3.start();
        sleep(100);
        t3.interrupt();
        sleep(100);
        t5.start();
        sleep(100);
        t5.interrupt();
        sleep(100);

        if (result == 2 && ThrowableExObjectwaitInterruptedException.res == 96) {
            result = 0;
        }

        return result;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n