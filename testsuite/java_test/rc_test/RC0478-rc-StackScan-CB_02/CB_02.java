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
class CB_02_A1 {
    static HashMap test1;
    static
    int a;
    CB_02_A2 a2_0;
    CB_02_A3 a3_0;
    int sum;
    String strObjectName;
    CB_02_A1(String strObjectName) {
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
        CB_02.check = this;
    }
}
class CB_02_A2 {
    volatile static HashMap test2;
    CB_02_A1 a1_0;
    CB_02_A3 a3_0;
    int a;
    int sum;
    String strObjectName;
    CB_02_A2(String strObjectName) {
        a1_0 = null;
        a3_0 = null;
        a = 102;
        sum = 0;
        this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A2_"+strObjectName);
    }
    void add() {
        sum = a + CB_02_A1.a + a3_0.a;
    }
    @Override
    public void finalize() throws Throwable {
        super.finalize();
    }
}
class CB_02_A3 {
    CB_02_A1 a1_0;
    CB_02_A2 a2_0;
    int a;
    int sum;
    String strObjectName;
    CB_02_A3(String strObjectName) {
        a1_0 = null;
        a2_0 = null;
        a = 103;
        sum = 0;
        this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A3_"+strObjectName);
    }
    void add() {
        sum = a + CB_02_A1.a + a2_0.a;
    }
}
public class CB_02 {
    public volatile static CB_02_A1 check = null;
    private static CB_02_A1 a1_main = null;
    private CB_02() {
        a1_main = new CB_02_A1("a1_main");
        a1_main.a2_0 = new CB_02_A2("a2_0");
        a1_main.a2_0.a1_0 = a1_main;
        a1_main.a2_0.a3_0 = new CB_02_A3("a3_0");
        a1_main.a3_0 = a1_main.a2_0.a3_0;
        a1_main.a3_0.a1_0 = a1_main;
        a1_main.a3_0.a2_0 = a1_main.a2_0;
        a1_main.add();
        a1_main.a2_0.add();
        a1_main.a2_0.a3_0.add();
    }
    private static void test_CB_02(int times) {
        CB_02_A1.test1 = new HashMap();
        CB_02_A2.test2 = new HashMap();
        for (int i = 0; i < times; i++) {
            CB_02_A1.test1.put(i, "Maple" + i);
            CB_02_A2.test2.put(i, "Figo" + i);
        }
    }
    private static void rc_testcase_main_wrapper() {
        CB_02 cb01 = new CB_02();
        test_CB_02(100000);
        check = a1_main;
        try {
            int result = CB_02.check.sum + CB_02.check.a2_0.sum + CB_02.check.a2_0.a3_0.sum + CB_02_A1.test1.size() + CB_02_A2.test2.size();
            if (result == 200918)
                System.out.println("ExpectResult");
        } catch (NullPointerException n) {
            System.out.println("ErrorResult");
        }
    }
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
    }
}
