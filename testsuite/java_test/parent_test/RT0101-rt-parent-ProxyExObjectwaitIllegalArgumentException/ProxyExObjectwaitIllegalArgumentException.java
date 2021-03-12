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


import java.lang.reflect.Proxy;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
public class ProxyExObjectwaitIllegalArgumentException {
    static int res = 99;
    private MyProxy5 proxy = new MyProxy5(new MyInvocationHandler5());
    public static void main(String argv[]) {
        System.out.println(new ProxyExObjectwaitIllegalArgumentException().run());
    }
    private class ProxyExObjectwaitIllegalArgumentException21 implements Runnable {
        // IllegalArgumentException - if the value of timeout is negative.
        // final void wait(long millis)
        private int remainder;
        long millis = -1;
        private ProxyExObjectwaitIllegalArgumentException21(int remainder) {
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
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 15;
                } catch (InterruptedException e1) {
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 20;
                } catch (IllegalMonitorStateException e2) {
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 5;
                }
            }
        }
    }
    private class ProxyExObjectwaitIllegalArgumentException31 implements Runnable {
        // IllegalArgumentException - if the value of timeout is negative or the value of nanos is not in the range 0-999999.
        //
        // final void wait(long millis, int nanos)
        private int remainder;
        long millis = -2;
        int nanos = 10;
        private ProxyExObjectwaitIllegalArgumentException31(int remainder) {
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
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 15;
                } catch (InterruptedException e1) {
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 20;
                } catch (IllegalMonitorStateException e2) {
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 10;
                } catch (IllegalArgumentException e3) {
                    ProxyExObjectwaitIllegalArgumentException.res = ProxyExObjectwaitIllegalArgumentException.res - 5;
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
        Thread t3 = new Thread(new ProxyExObjectwaitIllegalArgumentException21(3));
        // final void wait(long millis, int nanos)
        Thread t5 = new Thread(new ProxyExObjectwaitIllegalArgumentException31(5));
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
        if (result == 2 && ProxyExObjectwaitIllegalArgumentException.res == 89) {
            result = 0;
        }
        return result;
    }
}
class MyProxy5 extends Proxy {
    MyProxy5(InvocationHandler h) {
        super(h);
    }
    InvocationHandler getInvocationHandlerField() {
        return h;
    }
}
class MyInvocationHandler5 implements InvocationHandler {
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