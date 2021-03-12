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


class A2 {
    public int count = 0;
    public String className = "A";
    public A2(String name) {
        this.className = name;
    }
    public void changeName(String name) {
        this.className = name;
    }
}
public class LiverangeSubsumeRCTest11 {
    private volatile static int count = 0;
    public static void onlyUseInsideLoop() {
        A2 a1 = new A2("a1");
        for (count = 0; count < 100; count++) {
            A2 a2 = new A2("a2");
            a2.changeName("a" + count);
            a2.count = count;
        }
        if (count % 10 == 0) {
            a1.changeName("a" + 100);
            System.out.println(a1.className);
        }
    }
    public static void defAndUseInsideLoop() {
        for (count = 0; count < 10; count++) {
            A2 a1 = new A2("a1");
            a1.count = count;
            A2 a2 = a1;
            a2.changeName("null");
            if (count % 2 == 0) {
                a2.changeName("a" + 100);
                a2 = new A2("a10");
                a2.toString();
            } else {
                a1.changeName("a" + 100);
                a1.toString();
            }
        }
    }
    public static void main(String[] args) {
        onlyUseInsideLoop();
        defAndUseInsideLoop();
        new LiverangeSubsumeRCTest11().defInsideAndUseOutsideLoop();
    }
    public void defInsideAndUseOutsideLoop() {
        count = 0;
        do {
            int choice = count % 4;
            A2 a1 = new A2("a2_i" + count);
            A2 a2 = a1;
            switch (choice) {
                case 1:
                    a1.changeName("case 1");
                    break;
                case 2:
                    a2.changeName("case 2");
                    break;
                case 3:
                    break;
                default:
                    a1 = a2;
            }
            count++;
        } while (count < 10);
    }
}
