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
public class ClassExObjectwaitIllegalArgumentException {
    static int res = 99;
    private Class<?>[] clazz = ClassExObjectwaitIllegalArgumentException.class.getClasses();
    public static void main(String argv[]) {
        System.out.println(new ClassExObjectwaitIllegalArgumentException().run());
    }
    private class ClassExObjectwaitIllegalArgumentException21 implements Runnable {
        // IllegalArgumentException - if the value of timeout is negative.
        // final void wait(long millis)
        long millis = -1;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (clazz) {
                clazz.notifyAll();
                try {
                    clazz.wait(millis);
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 15;
                } catch (InterruptedException e1) {
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 20;
                } catch (IllegalMonitorStateException e2) {
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 5;
                }
            }
        }
    }
    private class ClassExObjectwaitIllegalArgumentException31 implements Runnable {
        // IllegalArgumentException - if the value of timeout is negative or the value of nanos is not in the range 0-999999.
        // final void wait(long millis, int nanos)
        long millis = -2;
        int nanos = 10;
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (clazz) {
                clazz.notifyAll();
                try {
                    clazz.wait(millis, nanos);
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 15;
                } catch (InterruptedException e1) {
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 20;
                } catch (IllegalMonitorStateException e2) {
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ClassExObjectwaitIllegalArgumentException.res = ClassExObjectwaitIllegalArgumentException.res - 5;
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
        // final void wait(long millis)
        Thread t3 = new Thread(new ClassExObjectwaitIllegalArgumentException21());
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ClassExObjectwaitIllegalArgumentException31());
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
        t3.start();
        t5.start();
        sleep(1000);
        if (result == 2 && ClassExObjectwaitIllegalArgumentException.res == 89) {
            result = 0;
        }
        return result;
    }
}
