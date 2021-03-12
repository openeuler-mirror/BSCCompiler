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


class RcThread0701 extends Thread {
    public void run() {
        RC_Thread_07 rcth01 = new RC_Thread_07();
        try {
            rcth01.modifyA1();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread0702 extends Thread {
    public void run() {
        RC_Thread_07 rcth01 = new RC_Thread_07();
        try {
            rcth01.checkA4();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread0703 extends Thread {
    public void run() {
        RC_Thread_07 rcth01 = new RC_Thread_07();
        try {
            rcth01.setA5();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}
class RcThread07GC extends Thread {
    public void run() {
        RC_Thread_07 rcth01 = new RC_Thread_07();
        int start = 0;
        do {
            Runtime.getRuntime().gc();
            start++;
        } while (start < 20);
    }
}
public class RC_Thread_07 {
    private static volatile RcThread07A1 a1_main = null;
    private static volatile RcThread07A5 a5_main = null;
    RC_Thread_07() {
        try {
            RcThread07A1 a1_1 = new RcThread07A1();
            a1_1.a2_0 = new RcThread07A2();
            a1_1.a2_0.a3_0 = new RcThread07A3();
            a1_1.a2_0.a3_0.a4_0 = new RcThread07A4();
            a1_1.a2_0.a3_0.a4_0.a1_0 = a1_1;
            RcThread07A5 a5_1 = new RcThread07A5();
            a1_1.a2_0.a3_0.a4_0.a6_0 = new RcThread07A6();
            a1_1.a2_0.a3_0.a4_0.a6_0.a5_0 = a5_1;
            a5_1.a3_0 = a1_1.a2_0.a3_0;
            a1_main = a1_1;
            a5_main = a5_1;
        } catch (NullPointerException e) {
            // do nothing
        }
    }
    public static void main(String[] args) {
        cyclePatternWrapper();
        Runtime.getRuntime().gc();
        cyclePatternWrapper();
        Runtime.getRuntime().gc();
        RcThread07GC gc = new RcThread07GC();
        gc.start();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        System.out.println("ExpectResult");
    }
    private static void cyclePatternWrapper() {
        RcThread07A1 a1 = new RcThread07A1();
        a1.a2_0 = new RcThread07A2();
        a1.a2_0.a3_0 = new RcThread07A3();
        a1.a2_0.a3_0.a4_0 = new RcThread07A4();
        a1.a2_0.a3_0.a4_0.a1_0 = a1;
        RcThread07A5 a5 = new RcThread07A5();
        a1.a2_0.a3_0.a4_0.a6_0 = new RcThread07A6();
        a1.a2_0.a3_0.a4_0.a6_0.a5_0 = a5;
        a5.a3_0 = a1.a2_0.a3_0;
        a1 = null;
        a5 = null;
        a1 = new RcThread07A1();
        a1.a2_0 = new RcThread07A2();
        a1.a2_0.a3_0 = new RcThread07A3();
        a1.a2_0.a3_0.a4_0 = new RcThread07A4();
        a1.a2_0.a3_0.a4_0.a1_0 = a1;
        a1 = null;
        a1 = new RcThread07A1();
        a1.a2_0 = new RcThread07A2();
        a1.a2_0.a3_0 = new RcThread07A3();
        a1.a2_0.a3_0.a4_0 = new RcThread07A4();
        a1.a2_0.a3_0.a4_0.a1_0 = a1;
        a5 = new RcThread07A5();
        a1.a2_0.a3_0.a4_0.a6_0 = new RcThread07A6();
        a1.a2_0.a3_0.a4_0.a6_0.a5_0 = a5;
        a5.a3_0 = a1.a2_0.a3_0;
        a1.a2_0.a3_0 = null;
        a1 = null;
        a5 = null;
    }
    private static void rcTestcaseMainWrapper() {
        RcThread0701 t1 = new RcThread0701();
        RcThread0702 t2 = new RcThread0702();
        RcThread0703 t3 = new RcThread0703();
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
    void modifyA1() {
        a1_main.a2_0.a3_0 = null;
        a1_main = null;
    }
    void checkA4() {
        try {
            int[] arr = new int[2];
            arr[0] = a5_main.a3_0.a4_0.sum;
            arr[1] = a5_main.a3_0.a4_0.a;
        } catch (NullPointerException e) {
            // do nothing
        }
    }
    void setA5() {
        RcThread07A5 a5 = new RcThread07A5();
        a5 = this.a5_main;
        a5_main = null;
    }
    static class RcThread07A1 {
        volatile RcThread07A2 a2_0;
        volatile RcThread07A4 a4_0;
        int a;
        int sum;
        RcThread07A1() {
            a2_0 = null;
            a4_0 = null;
            a = 1;
            sum = 0;
        }
        void add() {
            sum = a + a2_0.a;
        }
    }
    static class RcThread07A2 {
        volatile RcThread07A3 a3_0;
        int a;
        int sum;
        RcThread07A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
        void add() {
            sum = a + a3_0.a;
        }
    }
    static class RcThread07A3 {
        volatile RcThread07A4 a4_0;
        int a;
        int sum;
        RcThread07A3() {
            a4_0 = null;
            a = 3;
            sum = 0;
        }
        void add() {
            sum = a + a4_0.a;
        }
    }
    static class RcThread07A4 {
        volatile RcThread07A1 a1_0;
        volatile RcThread07A6 a6_0;
        int a;
        int sum;
        RcThread07A4() {
            a1_0 = null;
            a6_0 = null;
            a = 4;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a + a6_0.a;
        }
    }
    static class RcThread07A5 {
        volatile RcThread07A3 a3_0;
        int a;
        int sum;
        RcThread07A5() {
            a3_0 = null;
            a = 5;
            sum = 0;
        }
        void add() {
            sum = a + a3_0.a;
        }
    }
    static class RcThread07A6 {
        volatile RcThread07A5 a5_0;
        int a;
        int sum;
        RcThread07A6() {
            a5_0 = null;
            a = 6;
            sum = 0;
        }
        void add() {
            sum = a + a5_0.a;
        }
    }
}
