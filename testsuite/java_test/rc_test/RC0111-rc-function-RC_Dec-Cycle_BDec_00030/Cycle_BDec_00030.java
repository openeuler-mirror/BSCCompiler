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


class Cycle_BDec_00030_A1 {
    Cycle_BDec_00030_A2 a2_0;
    int a;
    int sum;
    Cycle_BDec_00030_A1() {
        a2_0 = null;
        a = 1;
        sum = 0;
    }
    void add() {
        sum = a + a2_0.a;
    }
}
class Cycle_BDec_00030_A2 {
    Cycle_BDec_00030_A3 a3_0;
    Cycle_BDec_00030_A4 a4_0;
    int a;
    int sum;
    Cycle_BDec_00030_A2() {
        a3_0 = null;
        a4_0 = null;
        a = 2;
        sum = 0;
    }
    void add() {
        sum = a + a3_0.a;
    }
}
class Cycle_BDec_00030_A3 {
    Cycle_BDec_00030_A1 a1_0;
    Cycle_BDec_00030_A4 a4_0;
    int a;
    int sum;
    Cycle_BDec_00030_A3() {
        a1_0 = null;
        a4_0 = null;
        a = 3;
        sum = 0;
    }
    void add() {
        sum = a + a4_0.a;
    }
}
class Cycle_BDec_00030_A4 {
    Cycle_BDec_00030_A3 a3_0;
    int a;
    int sum;
    Cycle_BDec_00030_A4() {
        a3_0 = null;
        a = 4;
        sum = 0;
    }
    void add() {
        sum = a + a3_0.a;
    }
}
public class Cycle_BDec_00030 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void ModifyA1(Cycle_BDec_00030_A1 a1) {
        a1 = null;
        a1 = new Cycle_BDec_00030_A1();
    }
    private static void rc_testcase_main_wrapper() {
        Cycle_BDec_00030_A1 a1_0 = new Cycle_BDec_00030_A1();
        a1_0.a2_0 = new Cycle_BDec_00030_A2();
        a1_0.a2_0.a3_0 = new Cycle_BDec_00030_A3();
        Cycle_BDec_00030_A4 a4_0 = new Cycle_BDec_00030_A4();
        a1_0.a2_0.a3_0.a1_0 = a1_0;
        a4_0.a3_0 = a1_0.a2_0.a3_0;
        a1_0.a2_0.a3_0.a4_0 = a4_0;
        ModifyA1(a1_0);
        a1_0 = null;
        a4_0.add();
        a4_0.a3_0.add();
        int nsum = (a4_0.a3_0.sum + a4_0.sum);
        if (nsum == 14)
            System.out.println("ExpectResult");
    }
}
