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


import com.huawei.ark.annotation.Weak;
class Cycle_B_1_00010_A1 {
    @Weak
    Cycle_B_1_00010_A1 a1_0;
    int a;
    int sum;
    Cycle_B_1_00010_A1() {
        a1_0 = null;
        a = 123;
        sum = 0;
    }
    void add() {
        sum = a + a1_0.a;
    }
}
public class RCWeakRefTest4 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        Cycle_B_1_00010_A1 a1_0 = new Cycle_B_1_00010_A1();
        a1_0.a1_0 = a1_0;
        a1_0.add();
        int nsum = a1_0.sum;
        if (nsum == 246)
            System.out.println("ExpectResult");
    }
}
