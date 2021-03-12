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


import java.lang.Character;
public class UnicodeBlockExObjectnotifyAllIllegalMonitorStateException {
    static int res = 99;
    private static Character.UnicodeBlock unb1 = null;
    public static void main(String argv[]) {
        for (int cp = 0; cp <= 10; ++cp) {
            unb1 = Character.UnicodeBlock.of(cp);
        }
        System.out.println(new UnicodeBlockExObjectnotifyAllIllegalMonitorStateException().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = unicodeBlockExObjectnotifyAllIllegalMonitorStateException1();
        } catch (Exception e) {
            UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res = UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res - 20;
        }
        Thread t1 = new Thread(new UnicodeBlockExObjectnotifyAllIllegalMonitorStateException11(1));
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
        if (result == 4 && UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res == 58) {
            result = 0;
        }
        return result;
    }
    private int unicodeBlockExObjectnotifyAllIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        //  final void notifyAll()
        try {
            unb1.notifyAll();
            UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res = UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res - 10;
        } catch (IllegalMonitorStateException e2) {
            UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res = UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private class UnicodeBlockExObjectnotifyAllIllegalMonitorStateException11 implements Runnable {
        //  final void notifyAll()
        private int remainder;
        private UnicodeBlockExObjectnotifyAllIllegalMonitorStateException11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (unb1) {
                try {
                    unb1.notifyAll();
                    UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res = UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res - 40;
                } catch (IllegalMonitorStateException e2) {
                    UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res = UnicodeBlockExObjectnotifyAllIllegalMonitorStateException.res - 30;
                }
            }
        }
    }
}