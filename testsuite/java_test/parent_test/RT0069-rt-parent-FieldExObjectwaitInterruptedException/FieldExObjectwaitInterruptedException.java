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


import java.lang.reflect.Field;
public class FieldExObjectwaitInterruptedException {
    static int res = 99;
    private static Field[] fields = FieldExObjectwaitInterruptedException.class.getDeclaredFields();
    public static void main(String argv[]) throws NoSuchFieldException, SecurityException {
        System.out.println(new FieldExObjectwaitInterruptedException().run());
    }
    private class FieldExObjectwaitInterruptedException11 implements Runnable {
        // final void wait()
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (fields[0]) {
                fields[0].notifyAll();
                try {
                    fields[0].wait();
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 10;
                } catch (InterruptedException e1) {
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 30;
                }
            }
        }
    }
    private class FieldExObjectwaitInterruptedException12 implements Runnable {
        // final void wait(long millis)
        long millis = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (fields[0]) {
                fields[0].notifyAll();
                try {
                    fields[0].wait(millis);
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 10;
                } catch (InterruptedException e1) {
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 30;
                }
            }
        }
    }
    private class FieldExObjectwaitInterruptedException13 implements Runnable {
        // final void wait(long millis, int nanos)
        long millis = 10;
        int nanos = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (fields[0]) {
                fields[0].notifyAll();
                try {
                    fields[0].wait(millis, nanos);
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 10;
                } catch (InterruptedException e1) {
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    FieldExObjectwaitInterruptedException.res = FieldExObjectwaitInterruptedException.res - 30;
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
        // final void wait()
        Thread t1 = new Thread(new FieldExObjectwaitInterruptedException11());
        Thread t2 = new Thread(new FieldExObjectwaitInterruptedException11());
        // final void wait(long millis)
        Thread t3 = new Thread(new FieldExObjectwaitInterruptedException12());
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new FieldExObjectwaitInterruptedException13());
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
        try {
            t1.join();
            t3.join();
            t5.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (result == 2 && FieldExObjectwaitInterruptedException.res == 69) {
            result = 0;
        }
        t2.interrupt();
        return result;
    }
}
