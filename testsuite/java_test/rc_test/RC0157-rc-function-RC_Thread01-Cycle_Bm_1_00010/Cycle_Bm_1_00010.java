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


class ThreadRc_Cycle_Bm_1_00010 extends Thread {
    private boolean checkout;
    public void run() {
        Cycle_B_1_00010_A1 a1_0 = new Cycle_B_1_00010_A1();
        a1_0.a1_0 = a1_0;
        a1_0.add();
        int nsum = a1_0.sum;
        //System.out.println(nsum);
        if (nsum == 246)
            checkout = true;
        //System.out.println(checkout);
    }
    public boolean check() {
        return checkout;
    }
    class Cycle_B_1_00010_A1 {
        Cycle_B_1_00010_A1 a1_0;
        int a;
        int sum;
        Cycle_B_1_00010_A1() {
            a1_0 = null;
            a = 123;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a;
        }
    }
}
public class Cycle_Bm_1_00010 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        ThreadRc_Cycle_Bm_1_00010 A1_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A2_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A3_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A4_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A5_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        ThreadRc_Cycle_Bm_1_00010 A6_Cycle_Bm_1_00010 = new ThreadRc_Cycle_Bm_1_00010();
        A1_Cycle_Bm_1_00010.start();
        A2_Cycle_Bm_1_00010.start();
        A3_Cycle_Bm_1_00010.start();
        A4_Cycle_Bm_1_00010.start();
        A5_Cycle_Bm_1_00010.start();
        A6_Cycle_Bm_1_00010.start();
        try {
            A1_Cycle_Bm_1_00010.join();
            A2_Cycle_Bm_1_00010.join();
            A3_Cycle_Bm_1_00010.join();
            A4_Cycle_Bm_1_00010.join();
            A5_Cycle_Bm_1_00010.join();
            A6_Cycle_Bm_1_00010.join();
        } catch (InterruptedException e) {
        }
        if (A1_Cycle_Bm_1_00010.check() && A2_Cycle_Bm_1_00010.check() && A3_Cycle_Bm_1_00010.check() && A4_Cycle_Bm_1_00010.check() && A5_Cycle_Bm_1_00010.check() && A6_Cycle_Bm_1_00010.check())
            System.out.println("ExpectResult");
    }
}
