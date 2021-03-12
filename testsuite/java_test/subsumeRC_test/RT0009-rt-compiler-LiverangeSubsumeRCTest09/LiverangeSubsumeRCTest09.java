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


public class LiverangeSubsumeRCTest09 {
    LiverangeSubsumeRCTest09 next;
    int a;
    int sum;
    String strObjectName;
    LiverangeSubsumeRCTest09(String strObjectName) {
        next = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    public static void main(String[] args) {
        LiverangeSubsumeRCTest09 test9 = new LiverangeSubsumeRCTest09("test9");
        test9.next = new LiverangeSubsumeRCTest09("test2");
        test9.next.next = new LiverangeSubsumeRCTest09("test3");
        test9.next.next.next = new LiverangeSubsumeRCTest09("test4");
        test9.next.next.next.next = new LiverangeSubsumeRCTest09("test5");
        test9.next.next.next.next = test9;
        test9.next.next.next.next.add();
        test9.add();
        if (test9.sum == 202) {
            System.out.println("ExpectResult");
        } else {
            System.out.println(test9.next.sum);
        }
    }
    void add() {
        sum = a + next.a;
    }
}