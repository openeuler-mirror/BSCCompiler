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


public class LiverangeSubsumeRCTest05 {
    public static void main(String[] args) {
        LiverangeSubsumeRCTest05_Node test1 = new LiverangeSubsumeRCTest05_Node("test1");
        LiverangeSubsumeRCTest05_Node test2 = new LiverangeSubsumeRCTest05_Node("test2");
        LiverangeSubsumeRCTest05_Node test3 = new LiverangeSubsumeRCTest05_Node("test3");
        LiverangeSubsumeRCTest05_Node test4 = new LiverangeSubsumeRCTest05_Node("test4");
        LiverangeSubsumeRCTest05_B1 b1 = new LiverangeSubsumeRCTest05_B1("b1");
        test1.b1_0 = b1;
        test2.b1_0 = b1;
        test3.b1_0 = b1;
        test4.b1_0 = b1;
        test1.b1_0.add();
        test2.b1_0.add();
        test3.b1_0.add();
        test4.b1_0.add();
        b1.add();
        int result = test1.sum + test2.sum + test3.sum + test4.sum + test4.b1_0.sum;
        if (result == 402) {
            System.out.println("ExpectResult");
        }
    }
}
class LiverangeSubsumeRCTest05_Node {
    LiverangeSubsumeRCTest05_B1 b1_0;
    int a;
    int sum;
    String strObjectName;
    LiverangeSubsumeRCTest05_Node(String strObjectName) {
        b1_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + b1_0.a;
    }
}
class LiverangeSubsumeRCTest05_B1 {
    int a;
    int sum;
    String strObjectName;
    LiverangeSubsumeRCTest05_B1(String strObjectName) {
        a = 201;
        sum = 0;
        this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_B1_"+strObjectName);
    }
    void add() {
        sum = a + a;
    }
}