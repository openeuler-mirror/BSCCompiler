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


class ThreadRc_Cycle_Bm_1_00200 extends Thread {
    private boolean checkout;
    public void run() {
        Cycle_B_1_00200_A1 a1_0 = new Cycle_B_1_00200_A1();
        a1_0.a2_0 = new Cycle_B_1_00200_A2();
        a1_0.a2_0.a3_0 = new Cycle_B_1_00200_A3();
        Cycle_B_1_00200_A4 a4_0 = new Cycle_B_1_00200_A4();
        Cycle_B_1_00200_A5 a5_0 = new Cycle_B_1_00200_A5();
        a1_0.a4_0 = a4_0;
        a1_0.a5_0 = a5_0;
        a1_0.a2_0.a3_0.a1_0 = a1_0;
        a1_0.add();
        a1_0.a2_0.add();
        a1_0.a2_0.a3_0.add();
        a4_0.add();
        a5_0.add();
        int nsum = (a1_0.sum + a1_0.a2_0.sum + a1_0.a2_0.a3_0.sum + a4_0.sum + a5_0.sum);
        //System.out.println(nsum);
        if (nsum == 38)
            checkout = true;
        //System.out.println(checkout);
    }
    public boolean check() {
        return checkout;
    }
    class Cycle_B_1_00200_A1 {
        Cycle_B_1_00200_A2 a2_0;
        Cycle_B_1_00200_A4 a4_0;
        Cycle_B_1_00200_A5 a5_0;
        int a;
        int sum;
        Cycle_B_1_00200_A1() {
            a2_0 = null;
            a4_0 = null;
            a5_0 = null;
            a = 1;
            sum = 0;
        }
        void add() {
            sum = a + a2_0.a;
        }
    }
    class Cycle_B_1_00200_A2 {
        Cycle_B_1_00200_A3 a3_0;
        int a;
        int sum;
        Cycle_B_1_00200_A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
        void add() {
            sum = a + a3_0.a;
        }
    }
    class Cycle_B_1_00200_A3 {
        Cycle_B_1_00200_A1 a1_0;
        int a;
        int sum;
        Cycle_B_1_00200_A3() {
            a1_0 = null;
            a = 3;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a;
        }
    }
    class Cycle_B_1_00200_A4 {
        int a;
        int sum;
        Cycle_B_1_00200_A4() {
            a = 5;
            sum = 0;
        }
        void add() {
            sum = a + 6;
        }
    }
    class Cycle_B_1_00200_A5 {
        int a;
        int sum;
        Cycle_B_1_00200_A5() {
            a = 7;
            sum = 0;
        }
        void add() {
            sum = a + 8;
        }
    }
}
public class Cycle_Bm_1_00200 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        ThreadRc_Cycle_Bm_1_00200 A1_Cycle_Bm_1_00200 = new ThreadRc_Cycle_Bm_1_00200();
        ThreadRc_Cycle_Bm_1_00200 A2_Cycle_Bm_1_00200 = new ThreadRc_Cycle_Bm_1_00200();
        ThreadRc_Cycle_Bm_1_00200 A3_Cycle_Bm_1_00200 = new ThreadRc_Cycle_Bm_1_00200();
        ThreadRc_Cycle_Bm_1_00200 A4_Cycle_Bm_1_00200 = new ThreadRc_Cycle_Bm_1_00200();
        ThreadRc_Cycle_Bm_1_00200 A5_Cycle_Bm_1_00200 = new ThreadRc_Cycle_Bm_1_00200();
        ThreadRc_Cycle_Bm_1_00200 A6_Cycle_Bm_1_00200 = new ThreadRc_Cycle_Bm_1_00200();
        A1_Cycle_Bm_1_00200.start();
        A2_Cycle_Bm_1_00200.start();
        A3_Cycle_Bm_1_00200.start();
        A4_Cycle_Bm_1_00200.start();
        A5_Cycle_Bm_1_00200.start();
        A6_Cycle_Bm_1_00200.start();
        try {
            A1_Cycle_Bm_1_00200.join();
            A2_Cycle_Bm_1_00200.join();
            A3_Cycle_Bm_1_00200.join();
            A4_Cycle_Bm_1_00200.join();
            A5_Cycle_Bm_1_00200.join();
            A6_Cycle_Bm_1_00200.join();
        } catch (InterruptedException e) {
        }
        if (A1_Cycle_Bm_1_00200.check() && A2_Cycle_Bm_1_00200.check() && A3_Cycle_Bm_1_00200.check() && A4_Cycle_Bm_1_00200.check() && A5_Cycle_Bm_1_00200.check() && A6_Cycle_Bm_1_00200.check())
            System.out.println("ExpectResult");
    }
}
