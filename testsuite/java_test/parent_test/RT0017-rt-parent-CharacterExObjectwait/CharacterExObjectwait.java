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
public class CharacterExObjectwait {
    static int res = 99;
    private char name = 'A';
    private String actions = null;
    private Character rp = new Character(name);
    private Character rp2 = new Character(name);
    private Character rp3 = new Character(name);
    public static void main(String argv[]) {
        System.out.println(new CharacterExObjectwait().run());
    }
    private class CharacterExObjectwait11 implements Runnable {
        // final void wait()
        private int remainder;
        private CharacterExObjectwait11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (rp) {
                rp.notifyAll();
                try {
                    rp.wait();
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 10;
                } catch (InterruptedException e1) {
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 30;
                }
            }
        }
    }
    private class CharacterExObjectwait12 implements Runnable {
        // final void wait(long millis)
        private int remainder;
        long millis = 10;
        private CharacterExObjectwait12(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (rp2) {
                rp2.notifyAll();
                try {
                    rp2.wait(millis);
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 10;
                } catch (InterruptedException e1) {
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 30;
                }
            }
        }
    }
    private class CharacterExObjectwait13 implements Runnable {
        // final void wait(long millis, int nanos)
        private int remainder;
        long millis = 10;
        int nanos = 10;
        private CharacterExObjectwait13(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (rp3) {
                rp3.notifyAll();
                try {
                    rp3.wait(millis, nanos);
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 10;
                } catch (InterruptedException e1) {
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    CharacterExObjectwait.res = CharacterExObjectwait.res - 30;
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
        Thread t1 = new Thread(new CharacterExObjectwait11(1));
        Thread t2 = new Thread(new CharacterExObjectwait11(2));
        // final void wait(long millis)
        Thread t3 = new Thread(new CharacterExObjectwait12(3));
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new CharacterExObjectwait13(5));
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
        sleep(100);
        t3.start();
        sleep(100);
        t5.start();
        sleep(100);
        t2.start();
        try {
            t1.join();
            t3.join();
            t5.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (result == 2 && CharacterExObjectwait.res == 69) {
            result = 0;
        }
        t2.interrupt();
        return result;
    }
}
