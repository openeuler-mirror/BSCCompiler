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


class RcThread0401 extends Thread {
    public void run() {
        RC_Thread_04 rcth01 = new RC_Thread_04();
        try {
            rcth01.setA1null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread0402 extends Thread {
    public void run() {
        RC_Thread_04 rcth01 = new RC_Thread_04();
        try {
            rcth01.setA4null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread0403 extends Thread {
    public void run() {
        RC_Thread_04 rcth01 = new RC_Thread_04();
        try {
            rcth01.setA5null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread04GC extends Thread {
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
public class RC_Thread_04 {
    private static volatile RcThread04A1 a1_main = null;
    private static volatile  RcThread04A4 a4_main = null;
    private static volatile  RcThread04A5 a5_main = null;
    RC_Thread_04() {
        try {
            RcThread04A1 a1 = new RcThread04A1();
            a1.a2_0 = new RcThread04A2();
            a1.a2_0.a3_0 = new RcThread04A3();
            RcThread04A4 a4 = new RcThread04A4();
            RcThread04A5 a5 = new RcThread04A5();
            a4.a1_0 = a1;
            a5.a1_0 = a1;
            a1.a2_0.a3_0.a1_0 = a1;
            a1_main = a1;
            a4_main = a4;
            a5_main = a5;
        } catch (NullPointerException e) {
            // do nothing
        }
    }
    public static void main(String[] args) {
        cyclePatternWrapper();
        RcThread04GC gc = new RcThread04GC();
        gc.start();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        System.out.println("ExpectResult");
    }
    private static void cyclePatternWrapper() {
        a1_main = new RcThread04A1();
        a1_main.a2_0 = new RcThread04A2();
        a1_main.a2_0.a3_0 = new RcThread04A3();
        a4_main = new RcThread04A4();
        a5_main = new RcThread04A5();
        a4_main.a1_0 = a1_main;
        a5_main.a1_0 = a1_main;
        a1_main.a2_0.a3_0.a1_0 = a1_main;
        a1_main = null;
        a4_main = null;
        a5_main = null;
        Runtime.getRuntime().gc(); // 单独构造一次Cycle_B_1_00180环，通过gc学习到环模式
    }
    private static void rcTestcaseMainWrapper() {
        RcThread0401 t1 = new RcThread0401();
        RcThread0402 t2 = new RcThread0402();
        RcThread0403 t3 = new RcThread0403();
        t1.start();
        t2.start();
        t3.start();
        try {
            t1.join();
            t2.join();
            t3.join();
        }catch(InterruptedException e){
            // do nothing
        }
    }
    void setA1null() {
        a1_main = null;
    }
    void setA4null() {
        a4_main = null;
    }
    void setA5null() {
        a5_main = null;
    }
    static class RcThread04A1 {
        volatile RcThread04A2 a2_0;
        int a;
        int sum;
        RcThread04A1() {
            a2_0 = null;
            a = 1;
            sum = 0;
        }
    }
    static class RcThread04A2 {
        volatile RcThread04A3 a3_0;
        int a;
        int sum;
        RcThread04A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
    }
    static class RcThread04A3 {
        volatile RcThread04A1 a1_0;
        int a;
        int sum;
        RcThread04A3() {
            a1_0 = null;
            a = 3;
            sum = 0;
        }
    }
    static class RcThread04A4 {
        volatile RcThread04A1 a1_0;
        int a;
        int sum;
        RcThread04A4() {
            a1_0 = null;
            a = 4;
            sum = 0;
        }
    }
    static class RcThread04A5 {
        volatile RcThread04A1 a1_0;
        int a;
        int sum;
        RcThread04A5() {
            a1_0 = null;
            a = 5;
            sum = 0;
        }
    }
}