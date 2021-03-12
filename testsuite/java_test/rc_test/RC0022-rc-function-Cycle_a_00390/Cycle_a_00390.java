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


class Cycle_a_00390_A1 {
    Cycle_a_00390_A2 a2_0;
    Cycle_a_00390_A3 a3_0;
    int a;
    int sum;
    String strObjectName;
    Cycle_a_00390_A1(String strObjectName) {
        a2_0 = null;
        a3_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A1_"+strObjectName);
    }
    void add() {
        sum = a + a2_0.a + a3_0.a;
    }
}
class Cycle_a_00390_A2 {
    Cycle_a_00390_A1 a1_0;
    Cycle_a_00390_A3 a3_0;
    int a;
    int sum;
    String strObjectName;
    Cycle_a_00390_A2(String strObjectName) {
        a1_0 = null;
        a3_0 = null;
        a = 102;
        sum = 0;
        this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A2_"+strObjectName);
    }
    void add() {
        sum = a + a1_0.a + a3_0.a;
    }
}
class Cycle_a_00390_A3 {
    Cycle_a_00390_A1 a1_0;
    Cycle_a_00390_A2 a2_0;
    int a;
    int sum;
    String strObjectName;
    Cycle_a_00390_A3(String strObjectName) {
        a1_0 = null;
        a2_0 = null;
        a = 103;
        sum = 0;
        this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A3_"+strObjectName);
    }
    void add() {
        sum = a + a1_0.a + a2_0.a;
    }
}
public class Cycle_a_00390 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        Cycle_a_00390_A1 a1_main = new Cycle_a_00390_A1("a1_main");
        a1_main.a2_0 = new Cycle_a_00390_A2("a2_0");
        a1_main.a2_0.a1_0 = a1_main;
        a1_main.a2_0.a3_0 = new Cycle_a_00390_A3("a3_0");
        a1_main.a3_0 = a1_main.a2_0.a3_0;
        a1_main.a3_0.a1_0 = a1_main;
        a1_main.a3_0.a2_0 = a1_main.a2_0;
        a1_main.add();
        a1_main.a2_0.add();
        a1_main.a2_0.a3_0.add();
        int result = a1_main.sum + a1_main.a2_0.sum + a1_main.a2_0.a3_0.sum;
        //System.out.println("RC-Testing_Result="+result);
        if (result == 918)
            System.out.println("ExpectResult");
    }
}
