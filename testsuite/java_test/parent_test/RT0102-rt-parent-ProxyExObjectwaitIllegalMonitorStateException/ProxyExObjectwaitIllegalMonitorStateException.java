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
public class ProxyExObjectwaitIllegalMonitorStateException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        // final void wait()
        try {
            result = proxyExObjectwaitIllegalMonitorStateException1();
        } catch (Exception e) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis)
        try {
            result = proxyExObjectwaitIllegalMonitorStateException2();
        } catch (Exception e) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 20;
        }
        // final void wait(long millis, int nanos)
        try {
            result = proxyExObjectwaitIllegalMonitorStateException3();
        } catch (Exception e) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 20;
        }
        if (result == 4 && ProxyExObjectwaitIllegalMonitorStateException.res == 96) {
            result = 0;
        }
        return result;
    }
    private static int proxyExObjectwaitIllegalMonitorStateException1() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait()
        MyProxy7 proxy = new MyProxy7(new MyInvocationHandler6());
        try {
            proxy.wait();
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int proxyExObjectwaitIllegalMonitorStateException2() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis)
        MyProxy7 proxy = new MyProxy7(new MyInvocationHandler6());
        long millis = 123;
        try {
            proxy.wait(millis);
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
    private static int proxyExObjectwaitIllegalMonitorStateException3() {
        int result1 = 4; /*STATUS_FAILED*/
        // IllegalMonitorStateException - if the current thread is not the owner of the object's monitor.
        // final void wait(long millis, int nanos)
        MyProxy7 proxy = new MyProxy7(new MyInvocationHandler6());
        long millis = 123;
        int nanos = 10;
        try {
            proxy.wait(millis, nanos);
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 10;
        } catch (InterruptedException e1) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 30;
        } catch (IllegalMonitorStateException e2) {
            ProxyExObjectwaitIllegalMonitorStateException.res = ProxyExObjectwaitIllegalMonitorStateException.res - 1;
        }
        return result1;
    }
}
class MyProxy7 extends Proxy {
    MyProxy7(InvocationHandler h) {
        super(h);
    }
    InvocationHandler getInvocationHandlerField() {
        return h;
    }
}
class MyInvocationHandler6 implements InvocationHandler {
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