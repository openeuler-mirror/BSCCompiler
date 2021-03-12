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


import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
public class ProxyExObjectwait {
    static int res = 99;
    private MyProxy4 proxy = new MyProxy4(new MyInvocationHandler3());
    public static void main(String argv[]) {
        System.out.println(new ProxyExObjectwait().run());
    }
    private class ProxyExObjectwait11 implements Runnable {
        // final void wait()
        private int remainder;
        private ProxyExObjectwait11(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (proxy) {
                proxy.notifyAll();
                try {
                    proxy.wait();
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 10;
                }
            }
        }
    }
    private class ProxyExObjectwait21 implements Runnable {
        // final void wait(long millis)
        private int remainder;
        long millis = 10;
        private ProxyExObjectwait21(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (proxy) {
                proxy.notifyAll();
                try {
                    proxy.wait(millis);
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 10;
                } catch (IllegalArgumentException e3) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 5;
                }
            }
        }
    }
    private class ProxyExObjectwait31 implements Runnable {
        // final void wait(long millis, int nanos)
        private int remainder;
        long millis = 10;
        int nanos = 10;
        private ProxyExObjectwait31(int remainder) {
            this.remainder = remainder;
        }
        /**
         * Thread run fun
        */

        public void run() {
            synchronized (proxy) {
                proxy.notifyAll();
                try {
                    proxy.wait(millis, nanos);
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 15;
                } catch (InterruptedException e1) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 1;
                } catch (IllegalMonitorStateException e2) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 10;
                } catch (IllegalArgumentException e3) {
                    ProxyExObjectwait.res = ProxyExObjectwait.res - 5;
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
     *
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
        Thread t1 = new Thread(new ProxyExObjectwait11(1));
        Thread t2 = new Thread(new ProxyExObjectwait11(2));
        // final void wait(long millis)
        Thread t3 = new Thread(new ProxyExObjectwait21(3));
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ProxyExObjectwait31(5));
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
        waitFinish(t1);
        waitFinish(t3);
        waitFinish(t5);
        t2.interrupt();
        if (result == 2 && ProxyExObjectwait.res == 54) {
            result = 0;
        }
        return result;
    }
}
class MyProxy4 extends Proxy {
    MyProxy4(InvocationHandler h) {
        super(h);
    }
    InvocationHandler getInvocationHandlerField() {
        return h;
    }
}
class MyInvocationHandler3 implements InvocationHandler {
    /**
     * invoke test
     *
     * @param proxy  proxy test
     * @param method method for test
     * @param args   object[] for test
     * @return any implementation
     * @throws Throwable exception
    */

    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        return new Object(); // any implementation
    }
}