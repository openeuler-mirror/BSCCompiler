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
interface ProxyGetProxyClass1_1 {
    default void test1() {
    }
    default void test2() {
    }
    default void test3() {
    }
    default void test4() {
    }
    default void test5() {
    }
    default void test6() {
    }
    default void test7() {
    }
    default void test8() {
    }
    default void test9() {
    }
    default void test10() {
    }
    default void test11() {
    }
    default void test12() {
    }
    default void test13() {
    }
    default void test14() {
    }
    default void test15() {
    }
    default void test16() {
    }
    default void test17() {
    }
    default void test18() {
    }
    default void test19() {
    }
    default void test20() {
    }
    default void test21() {
    }
    default void test22() {
    }
    default void test23() {
    }
    default void test24() {
    }
    default void test25() {
    }
    default void test26() {
    }
    default void test27() {
    }
    default void test28() {
    }
    default void test29() {
    }
    default void test30() {
    }
}
interface ProxyGetProxyClass1_2 extends ProxyGetProxyClass1_1 {
    default void test30() {
    }
}
interface ProxyGetProxyClass1_3 {
    default void test31() {
    }
}
public class RTProxyGetProxyClass1 {
    public static void main(String[] args) {
        try {
            Proxy.getProxyClass(ProxyGetProxyClass1_1.class.getClassLoader(), new Class[] {ProxyGetProxyClass1_1.class,
                    ProxyGetProxyClass1_2.class});
            Proxy.getProxyClass(ProxyGetProxyClass1_1.class.getClassLoader(), new Class[] {ProxyGetProxyClass1_1.class,
                    ProxyGetProxyClass1_3.class});
        } catch (IllegalArgumentException e) {
            System.err.println(e);
        }
        System.out.println(0);
    }
}