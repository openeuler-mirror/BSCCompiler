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
public class StringExObjectnotifyAllIllegalMonitorStateException {
    static int res = 99;
    String mf2 = "this is a test";
    public static void main(String argv[]) {
        System.out.println(new StringExObjectnotifyAllIllegalMonitorStateException().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = stringExObjectnotifyAllIllegalMonitorStateException1();
        } catch (Exception e) {
            StringExObjectnotifyAllIllegalMonitorStateException.res = StringExObjectnotifyAllIllegalMonitorStateException.res - 20;
        }
        Thread t1 = new Thread(new StringExObjectnotifyAllIllegalMonitorStateException11());
        t1.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                System.out.println(t.getName() + " : " + e.getMessage());
            }
        });
        t1.start();
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (result == 4 && StringExObjectnotifyAllIllegalMonitorStateException.res == 58) {
            result = 0;
        }
        return result;
    }
    private int stringExObjectnotifyAllIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        //  final void notifyAll()
        try {
            mf2.notifyAll();
            StringExObjectnotifyAllIllegalMonitorStateException.res = StringExObjectnotifyAllIllegalMonitorStateException.res - 10;
        } catch (IllegalMonitorStateException e2) {
            StringExObjectnotifyAllIllegalMonitorStateException.res = StringExObjectnotifyAllIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private class StringExObjectnotifyAllIllegalMonitorStateException11 implements Runnable {
        //  final void notifyAll()
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (mf2) {
                try {
                    mf2.notifyAll();
                    StringExObjectnotifyAllIllegalMonitorStateException.res = StringExObjectnotifyAllIllegalMonitorStateException.res - 40;
                } catch (IllegalMonitorStateException e2) {
                    StringExObjectnotifyAllIllegalMonitorStateException.res = StringExObjectnotifyAllIllegalMonitorStateException.res - 30;
                }
            }
        }
    }
}