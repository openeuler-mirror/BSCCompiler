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


class A3 {
    public int count = 0;
    public String className = "temp";
    public A3(String name) {
        this.className = name;
    }
    public void changeName(String name) {
        this.className = name;
    }
}
public class LiverangeSubsumeRCTest12 {
    private volatile static int count = 0;
    private A3 defInsideUseOutside = null;
    //def outside, only use inside loop
    public static void onlyUseInsideLoop() {
        A3 a1 = new A3("a1");
        continueTag:
        for (count = 0; count < 10; count++) {
            A3 a3 = a1;
            a1.changeName("a" + count);
            a3.count = count;
            if (count % 8 == 0) {
                a1.changeName("Right"); // a3没有incRef和decRef
                continue continueTag;
            } else {
                a1.className = "a1_" + count; // a3没有incRef和decRef
                continue;
            }
        }
    }
    public static A3 defAndUseInsideLoop() {
        for (count = 0; count < 10; count++) {
            A3 a2 = new A3("a2_i" + count);
            A3 a3 = a2; //因为else分支，这边会有一个incRef
            a2.count = count;
            a3.changeName("null");
            if (count % 4 == 0 && count > 0) {
                a2.changeName("Optimization");
                return a2;
            } else if (count == 3) {
                a3.changeName("NoOptimization");
                return a3;
            }
        }
        return new A3("Error");
    }
    public static void main(String[] args) {
        onlyUseInsideLoop();
        if (defAndUseInsideLoop().className.equals("NoOptimization")) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}