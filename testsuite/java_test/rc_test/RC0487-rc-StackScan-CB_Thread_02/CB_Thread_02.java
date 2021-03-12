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


import java.util.HashMap;
class CB_Thread_02_A1 {
    static HashMap test1;
    static
    int a;
    CB_Thread_02_A2 a2_0;
    CB_Thread_02_A3 a3_0;
    int sum;
    String strObjectName;
    CB_Thread_02_A1(String strObjectName) {
        a2_0 = null;
        a3_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + a2_0.a + a3_0.a;
    }
    @Override
    public void finalize() throws Throwable {
        super.finalize();
        CB_Thread_02_test.check = this;
    }
}
class CB_Thread_02_A2 {
    volatile static HashMap test2;
    CB_Thread_02_A1 a1_0;
    CB_Thread_02_A3 a3_0;
    int a;
    int sum;
    String strObjectName;
    CB_Thread_02_A2(String strObjectName) {
        a1_0 = null;
        a3_0 = null;
        a = 102;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + CB_Thread_02_A1.a + a3_0.a;
    }
    public void finalize() throws Throwable {
        super.finalize();
    }
}
class CB_Thread_02_A3 {
    CB_Thread_02_A1 a1_0;
    CB_Thread_02_A2 a2_0;
    int a;
    int sum;
    String strObjectName;
    CB_Thread_02_A3(String strObjectName) {
        a1_0 = null;
        a2_0 = null;
        a = 103;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + CB_Thread_02_A1.a + a2_0.a;
    }
}
class CB_Thread_02_test extends Thread {
    public volatile static CB_Thread_02_A1 check = null;
    private static CB_Thread_02_A1 a1_main = null;
    private static void test_CB_Thread_02(int times) {
        CB_Thread_02_A1.test1 = new HashMap();
        CB_Thread_02_A2.test2 = new HashMap();
        for (int i = 0; i < times; i++) {
            CB_Thread_02_A1.test1.put(i, "Maple" + i);
            CB_Thread_02_A2.test2.put(i, "Figo" + i);
        }
    }
    public void run() {
        a1_main = new CB_Thread_02_A1("a1_main");
        a1_main.a2_0 = new CB_Thread_02_A2("a2_0");
        a1_main.a2_0.a1_0 = a1_main;
        a1_main.a2_0.a3_0 = new CB_Thread_02_A3("a3_0");
        a1_main.a3_0 = a1_main.a2_0.a3_0;
        a1_main.a3_0.a1_0 = a1_main;
        a1_main.a3_0.a2_0 = a1_main.a2_0;
        test_CB_Thread_02(10000);
        a1_main = null;
        CB_Thread_02_test.check = null;
    }
}
public class CB_Thread_02 {
    private static CB_Thread_02_A1 a1_main = null;
    public static void main(String[] args) {
        a1_main = new CB_Thread_02_A1("a1_main");
        a1_main.a2_0 = new CB_Thread_02_A2("a2_0");
        a1_main.a2_0.a1_0 = a1_main;
        a1_main.a2_0.a3_0 = new CB_Thread_02_A3("a3_0");
        a1_main.a3_0 = a1_main.a2_0.a3_0;
        a1_main.a3_0.a1_0 = a1_main;
        a1_main.a3_0.a2_0 = a1_main.a2_0;
        a1_main.add();
        a1_main.a2_0.add();
        a1_main.a2_0.a3_0.add();
        a1_main = null;
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        try {
            int result = CB_Thread_02_test.check.sum + CB_Thread_02_test.check.a2_0.sum +
                    CB_Thread_02_test.check.a2_0.a3_0.sum;
            if (result == 918) {
                System.out.println("ExpectResult");
            }
        } catch (NullPointerException n) {
            System.out.println("ErrorResult");
        }
    }
}
