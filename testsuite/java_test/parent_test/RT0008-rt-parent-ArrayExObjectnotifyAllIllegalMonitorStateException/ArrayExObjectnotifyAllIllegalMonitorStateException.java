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


import java.lang.reflect.Array;
public class ArrayExObjectnotifyAllIllegalMonitorStateException {
    static int res = 99;
    private static Object ary1 = Array.newInstance(int.class, 10);
    public static void main(String argv[]) {
        System.out.println(new ArrayExObjectnotifyAllIllegalMonitorStateException().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = arrayExObjectnotifyAllIllegalMonitorStateException1();
        } catch (Exception e) {
            ArrayExObjectnotifyAllIllegalMonitorStateException.res = ArrayExObjectnotifyAllIllegalMonitorStateException.res - 20;
        }
        Thread t1 = new Thread(new ArrayExObjectnotifyAllIllegalMonitorStateException11(1));
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t1.start();
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (result == 4 && ArrayExObjectnotifyAllIllegalMonitorStateException.res == 58) {
            result = 0;
        }
        return result;
    }
    private int arrayExObjectnotifyAllIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void notifyAll()
        try {
            ary1.notifyAll();
            ArrayExObjectnotifyAllIllegalMonitorStateException.res = ArrayExObjectnotifyAllIllegalMonitorStateException.res - 10;
        } catch (IllegalMonitorStateException e2) {
            ArrayExObjectnotifyAllIllegalMonitorStateException.res = ArrayExObjectnotifyAllIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private class ArrayExObjectnotifyAllIllegalMonitorStateException11 implements Runnable {
        // final void notifyAll()
        private int remainder;
        private ArrayExObjectnotifyAllIllegalMonitorStateException11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (ary1) {
                try {
                    ary1.notifyAll();
                    ArrayExObjectnotifyAllIllegalMonitorStateException.res = ArrayExObjectnotifyAllIllegalMonitorStateException.res - 40;
                } catch (IllegalMonitorStateException e2) {
                    ArrayExObjectnotifyAllIllegalMonitorStateException.res = ArrayExObjectnotifyAllIllegalMonitorStateException.res - 30;
                }
            }
        }
    }
}