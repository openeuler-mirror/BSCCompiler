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


class C {
    static A a;
}
class A {
    B b;
    public A(B b) {
        this.b = b;
    }
    @Override
    public void finalize() throws Throwable {
        super.finalize();
//        System.out.println("A finalize");
        C.a = this;
    }
}
class B {
    int num1;
    int num2;
    public B(int num1, int num2) {
        this.num1 = num1;
        this.num2 = num2;
    }
    @Override
    public void finalize() throws Throwable {
        super.finalize();
//        System.out.println("B finalize");
    }
    public int sum() {
        return num1 + num2;
    }
}
public class RC_Finalize_01 {
    public static void main(String[] args) throws Exception {
        A a = new A(new B(12, 18));
        a = null;
        //System.gc();
        Thread.sleep(5000);
		System.out.println("ExpectResult");
}
}
