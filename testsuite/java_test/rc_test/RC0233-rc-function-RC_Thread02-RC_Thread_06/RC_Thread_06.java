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


class RcThread0601 extends Thread {
    public void run() {
        RC_Thread_06 rcth01 = new RC_Thread_06();
        try {
            rcth01.setA1null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread0602 extends Thread {
    public void run() {
        RC_Thread_06 rcth01 = new RC_Thread_06();
        try {
            rcth01.setA4null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread0603 extends Thread {
    public void run() {
        RC_Thread_06 rcth01 = new RC_Thread_06();
        try {
            rcth01.setA5null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread06GC extends Thread {
    public void run() {
        int start = 0;
        do {
            Runtime.getRuntime().gc();
            start++;
        } while (start < 50);
        if (start == 50) {
            System.out.println("ExpectResult");
        }
    }
}
public class RC_Thread_06 {
    private static volatile RcThread06A1 a1Main = null;
    private static volatile RcThread06A5 a5Main = null;
    RC_Thread_06() {
        try {
            a1Main = new RcThread06A1();
            a1Main.a2_0 = new RcThread06A2();
            a1Main.a2_0.a3_0 = new RcThread06A3();
            a1Main.a2_0.a3_0.a4_0 = new RcThread06A4();
            a1Main.a2_0.a3_0.a4_0.a1_0 = a1Main;
            a5Main = new RcThread06A5();
            a1Main.a2_0.a3_0.a4_0.a6_0 = new RcThread06A6();
            a1Main.a2_0.a3_0.a4_0.a6_0.a5_0 = a5Main;
            a5Main.a3_0 = a1Main.a2_0.a3_0;
        } catch (NullPointerException e) {
            // do nothing
        }
    }
    public static void main(String[] args) {
        cyclePatternWrapper();
        Runtime.getRuntime().gc();
        RcThread06GC gc = new RcThread06GC();
        gc.run();
        int start = 0;
        for (;start < 20; start++) {
            rcTestcaseMainWrapper();
        }
        if (start == 20) {
            System.out.println("ExpectResult");
        }
    }
    private static void cyclePatternWrapper() {
        a1Main = new RcThread06A1();
        a1Main.a2_0 = new RcThread06A2();
        a1Main.a2_0.a3_0 = new RcThread06A3();
        a1Main.a2_0.a3_0.a4_0 = new RcThread06A4();
        a1Main.a2_0.a3_0.a4_0.a1_0 = a1Main;
        a5Main = new RcThread06A5();
        a1Main.a2_0.a3_0.a4_0.a6_0 = new RcThread06A6();
        a1Main.a2_0.a3_0.a4_0.a6_0.a5_0 = a5Main;
        a5Main.a3_0 = a1Main.a2_0.a3_0;
        a1Main = null;
        a5Main = null;
        a1Main = new RcThread06A1();
        a1Main.a2_0 = new RcThread06A2();
        a1Main.a2_0.a3_0 = new RcThread06A3();
        a1Main.a2_0.a3_0.a4_0 = new RcThread06A4();
        a1Main.a2_0.a3_0.a4_0.a1_0 = a1Main;
        a1Main = null;
        a1Main = new RcThread06A1();
        a1Main.a2_0 = new RcThread06A2();
        a1Main.a2_0.a3_0 = new RcThread06A3();
        a1Main.a2_0.a3_0.a4_0 = new RcThread06A4();
        a1Main.a2_0.a3_0.a4_0.a1_0 = a1Main;
        a5Main = new RcThread06A5();
        a1Main.a2_0.a3_0.a4_0.a6_0 = new RcThread06A6();
        a1Main.a2_0.a3_0.a4_0.a6_0.a5_0 = a5Main;
        a5Main.a3_0 = a1Main.a2_0.a3_0;
        a1Main.a2_0.a3_0 = null;
        a1Main = null;
        a5Main = null;     //总共三种环类型
    }
    private static void rcTestcaseMainWrapper() {
        RcThread0601 t1 = new RcThread0601();
        RcThread0602 t2 = new RcThread0602();
        RcThread0603 t3 = new RcThread0603();
        t1.start();
        t2.start();
        t3.start();
        try {
            t1.join();
            t2.join();
            t3.join();
        } catch (InterruptedException e){
            // do nothing
        }
    }
    void setA1null() {
        a1Main = null;
    }
    void setA4null() {
        try {
            a1Main.a2_0.a3_0.a4_0 = null;
            a5Main.a3_0.a4_0 = null;
        } catch (NullPointerException e) { }
    }
    void setA5null() {
        a5Main = null;
    }
    static class RcThread06A1 {
        volatile RcThread06A2 a2_0;
        volatile RcThread06A4 a4_0;
        int a;
        int sum;
        RcThread06A1() {
            a2_0 = null;
            a4_0 = null;
            a = 1;
            sum = 0;
        }
    }
    static class RcThread06A2 {
        volatile RcThread06A3 a3_0;
        int a;
        int sum;
        RcThread06A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
    }
    static class RcThread06A3 {
        volatile RcThread06A4 a4_0;
        int a;
        int sum;
        RcThread06A3() {
            a4_0 = null;
            a = 3;
            sum = 0;
        }
    }
    static class RcThread06A4 {
        volatile RcThread06A1 a1_0;
        volatile RcThread06A6 a6_0;
        int a;
        int sum;
        RcThread06A4() {
            a1_0 = null;
            a6_0 = null;
            a = 4;
            sum = 0;
        }
    }
    static class RcThread06A5 {
        volatile RcThread06A3 a3_0;
        int a;
        int sum;
        RcThread06A5() {
            a3_0 = null;
            a = 5;
            sum = 0;
        }
    }
    static class RcThread06A6 {
        volatile RcThread06A5 a5_0;
        int a;
        int sum;
        RcThread06A6() {
            a5_0 = null;
            a = 6;
            sum = 0;
        }
    }
}