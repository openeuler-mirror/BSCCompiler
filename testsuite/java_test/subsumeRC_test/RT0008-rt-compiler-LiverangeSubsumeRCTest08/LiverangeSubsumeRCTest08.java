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


public class LiverangeSubsumeRCTest08 {
    LiverangeSubsumeRCTest08 next;
    int a;
    int sum;
    String strObjectName;
    LiverangeSubsumeRCTest08(String strObjectName) {
        next = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    public static void main(String[] args) {
        LiverangeSubsumeRCTest08 test1 = new LiverangeSubsumeRCTest08("test1");
        test1.next = new LiverangeSubsumeRCTest08("test2");
        test1.next.next = test1;
        test1.next.add();
        test1.next.next.add();
        test1.add();
        if (test1.sum == 202) {
            System.out.println("ExpectResult");
        } else {
            System.out.println(test1.sum);
        }
    }
    void add() {
        sum = a + next.a;
    }
}