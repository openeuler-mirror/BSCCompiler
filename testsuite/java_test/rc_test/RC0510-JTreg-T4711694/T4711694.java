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


public class T4711694 {
    public static void main(String[] args) {
        D.main(args);
        System.out.println("ExpectResult");
    }
    interface A<T> {
        void f(T u);
    }
    static class B {
        public void f(Integer i) {
        }
    }
    static abstract class C<T> extends B implements A<T> {
        public void g(T t) {
            f(t);
        }
    }
    static class D extends C<Integer> {
        public static void main(String[] args) {
            new D().g(new Integer(3));
        }
    }
}