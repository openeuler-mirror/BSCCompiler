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


class Nocycle_a_00120_A1 {
    Nocycle_a_00120_B1 b1_0;
    int a;
    int sum;
    String strObjectName;
    Nocycle_a_00120_A1(String strObjectName) {
        b1_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + b1_0.a;
    }
}
class Nocycle_a_00120_A2 {
    Nocycle_a_00120_B1 b1_0;
    int a;
    int sum;
    String strObjectName;
    Nocycle_a_00120_A2(String strObjectName) {
        b1_0 = null;
        a = 102;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + b1_0.a;
    }
}
class Nocycle_a_00120_B1 {
    int a;
    int sum;
    String strObjectName;
    Nocycle_a_00120_B1(String strObjectName) {
        a = 201;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + a;
    }
}
public class Nocycle_a_00120 {
    public static void main(String[] args) {
        Nocycle_a_00120_A1 a1_main = new Nocycle_a_00120_A1("a1_main");
        Nocycle_a_00120_A2 a2_main = new Nocycle_a_00120_A2("a2_main");
        a1_main.b1_0 = new Nocycle_a_00120_B1("b1_0");
        a2_main.b1_0 = new Nocycle_a_00120_B1("b1_0");
        a1_main.add();
        a2_main.add();
        a1_main.b1_0.add();
        a2_main.b1_0.add();
        int result = a1_main.sum + a2_main.sum + a1_main.b1_0.sum;
        if (result == 1007) {
            System.out.println("ExpectResult");
        }
    }
}
