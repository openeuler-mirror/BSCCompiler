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


import java.lang.StringBuilder;
public class StringBuilderExObjectnotifyIllegalMonitorStateException {
    static int res = 99;
    static private String[] stringArray = {
            "", "a", "b", "c", "ab", "ac", "abc", "aaaabbbccc"
    };
    static private StringBuilder sb = null;
    public static void main(String argv[]) {
        for (int i = 0; i < stringArray.length; i++) {
            sb = new StringBuilder(stringArray[i]);
        }
        System.out.println(new StringBuilderExObjectnotifyIllegalMonitorStateException().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = stringBuilderExObjectnotifyIllegalMonitorStateException1();
        } catch (Exception e) {
            StringBuilderExObjectnotifyIllegalMonitorStateException.res = StringBuilderExObjectnotifyIllegalMonitorStateException.res - 20;
        }
        Thread t1 = new Thread(new StringBuilderExObjectnotifyIllegalMonitorStateException11(1));
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
        if (result == 4 && StringBuilderExObjectnotifyIllegalMonitorStateException.res == 58) {
            result = 0;
        }
        return result;
    }
    private int stringBuilderExObjectnotifyIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void notify()
        try {
            sb.notify();
            StringBuilderExObjectnotifyIllegalMonitorStateException.res = StringBuilderExObjectnotifyIllegalMonitorStateException.res - 10;
        } catch (IllegalMonitorStateException e2) {
            StringBuilderExObjectnotifyIllegalMonitorStateException.res = StringBuilderExObjectnotifyIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private class StringBuilderExObjectnotifyIllegalMonitorStateException11 implements Runnable {
        // final void notify()
        private int remainder;
        private StringBuilderExObjectnotifyIllegalMonitorStateException11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (sb) {
                try {
                    sb.notify();
                    StringBuilderExObjectnotifyIllegalMonitorStateException.res = StringBuilderExObjectnotifyIllegalMonitorStateException.res - 40;
                } catch (IllegalMonitorStateException e2) {
                    StringBuilderExObjectnotifyIllegalMonitorStateException.res = StringBuilderExObjectnotifyIllegalMonitorStateException.res - 30;
                }
            }
        }
    }
}