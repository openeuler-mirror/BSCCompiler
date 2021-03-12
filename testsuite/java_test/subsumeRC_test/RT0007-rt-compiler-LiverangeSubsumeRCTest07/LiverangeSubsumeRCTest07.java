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


public class LiverangeSubsumeRCTest07 {
    LiverangeSubsumeRCTest07 next;
    int a;
    int sum;
    String strObjectName;
    LiverangeSubsumeRCTest07(String strObjectName) {
        next = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    public static void main(String[] args) {
        LiverangeSubsumeRCTest07 test07 = new LiverangeSubsumeRCTest07("test1");
        test07.next = test07; // 使用intrinsiccall MCCWriteNoDec（）代替
        test07.next.add();
        test07.add();
        if (test07.sum == 202) {
            System.out.println("ExpectResult");
        } else {
            System.out.println(test07.sum);
        }
    }
    void add() {
        sum = a + next.a;
    }
}
