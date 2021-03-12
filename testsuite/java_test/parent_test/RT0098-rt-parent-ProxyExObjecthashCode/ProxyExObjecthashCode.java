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
public class ProxyExObjecthashCode {
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
        try {
            result = proxyExObjecthashCode1();
        } catch (Exception e) {
            ProxyExObjecthashCode.res = ProxyExObjecthashCode.res - 20;
        }
        if (result == 4 && ProxyExObjecthashCode.res == 89) {
            result = 0;
        }
        return result;
    }
    private static int proxyExObjecthashCode1() {
        //  int hashCode()
        int result1 = 4; /*STATUS_FAILED*/
        MyProxy1 px0 = new MyProxy1(new MyInvocationHandler());
        MyProxy1 px1 = px0;
        MyProxy1 px2 = new MyProxy1(new MyInvocationHandler());
        try {
            int int0 = px0.hashCode();
            int int1 = px1.hashCode();
            int int2 = px2.hashCode();
            if (px0 == px1 && px0 != px2) {
                ProxyExObjecthashCode.res = ProxyExObjecthashCode.res - 10;
            }
        } catch (IllegalMonitorStateException e) {
            ProxyExObjecthashCode.res = ProxyExObjecthashCode.res - 1;
        }
        return result1;
    }
}
class MyProxy1 extends Proxy {
    MyProxy1(InvocationHandler h) {
        super(h);
    }
    InvocationHandler getInvocationHandlerField() {
        return h;
    }
}
class MyInvocationHandler implements InvocationHandler {
    /**
     * invoke test
     * @param proxy proxy test
     * @param method method for test
     * @param args object[] for test
     * @return any implementation
     * @throws Throwable exception
    */

    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        return new Object(); // any implementation
    }
}