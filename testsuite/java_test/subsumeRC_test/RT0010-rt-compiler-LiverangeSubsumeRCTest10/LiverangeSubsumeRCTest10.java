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


class A {
    public int count = 0;
    public String className = "A";
    public A(String name) {
        this.className = name;
    }
    public void changeName(String name) {
        this.className = name;
    }
}
public class LiverangeSubsumeRCTest10 {
    private static volatile int count = 0;
    private static A infiniteLoop = null;
    private A defInsideUseOutside = null;
    public static boolean onlyUseInsideLoop() {
        A a1 = new A("a1");
        for (count = 0; count < 100; count++) {
            A a2 = a1;
            a2.changeName("a" + count);
            a2.count = count;
            if (count == 99)
                a2.toString();
        }
        return a1.className.equals("a99");
    }
    public static void defAndUseInsideLoop() {
        for (count = 0; count < 10; count++) {
            A a1 = new A("a1_" + count);
            a1.changeName("a1");
            A a2 = a1;
            for (int j = 0; j < 2; j++) {
                a2.changeName("a1_" + j);
            }
            System.out.print(a1.className);
        }
    }
    public static void main(String[] args) {
        defAndUseInsideLoop();
        new LiverangeSubsumeRCTest10().defInsideAndUseOutsideLoop();
        if (onlyUseInsideLoop()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
    public void defInsideAndUseOutsideLoop() {
        count = 0;
        do {
            this.defInsideUseOutside = new A("a1_i" + count);
            A a2 = this.defInsideUseOutside;
            a2.count = count;
            for (int j = 0; j < 2; j++)
                a2 = new A("a2_i" + count + "_j" + j);
            if (count == 99)
                a2.toString();
            count++;
        } while (this.defInsideUseOutside.count < 100 && count < 100);
        System.out.print(count);
    }
}
