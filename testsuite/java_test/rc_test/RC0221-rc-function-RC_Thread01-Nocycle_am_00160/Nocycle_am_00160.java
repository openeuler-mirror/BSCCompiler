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


class ThreadRc_00160 extends Thread {
    private boolean checkout;
    public void run() {
        Nocycle_a_00160_A1 a1_main = new Nocycle_a_00160_A1("a1_main");
        Nocycle_a_00160_A2 a2_main = new Nocycle_a_00160_A2("a2_main");
        Nocycle_a_00160_A3 a3_main = new Nocycle_a_00160_A3("a3_main");
        Nocycle_a_00160_A4 a4_main = new Nocycle_a_00160_A4("a4_main");
        a1_main.b1_0 = new Nocycle_a_00160_B1("b1_0");
        a1_main.b2_0 = new Nocycle_a_00160_B2("b2_0");
        a2_main.b1_0 = new Nocycle_a_00160_B1("b1_0");
        a2_main.b2_0 = new Nocycle_a_00160_B2("b2_0");
        a3_main.b1_0 = new Nocycle_a_00160_B1("b1_0");
        a3_main.b2_0 = new Nocycle_a_00160_B2("b2_0");
        a4_main.b1_0 = new Nocycle_a_00160_B1("b1_0");
        a4_main.b2_0 = new Nocycle_a_00160_B2("b2_0");
        a1_main.add();
        a2_main.add();
        a3_main.add();
        a4_main.add();
        a1_main.b1_0.add();
        a1_main.b2_0.add();
        a2_main.b1_0.add();
        a2_main.b2_0.add();
        a3_main.b1_0.add();
        a3_main.b2_0.add();
        a4_main.b1_0.add();
        a4_main.b2_0.add();
//		 System.out.printf("RC-Testing_Result=%d\n",a1_main.sum+a2_main.sum+a3_main.sum+a4_main.sum+a1_main.b1_0.sum+a1_main.b2_0.sum);
        int result = a1_main.sum + a2_main.sum + a3_main.sum + a4_main.sum + a1_main.b1_0.sum + a1_main.b2_0.sum;
        //System.out.println("RC-Testing_Result="+result);
        if (result == 2828)
            checkout = true;
        //System.out.println(checkout);
    }
    public boolean check() {
        return checkout;
    }
    class Nocycle_a_00160_A1 {
        Nocycle_a_00160_B1 b1_0;
        Nocycle_a_00160_B2 b2_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00160_A1(String strObjectName) {
            b1_0 = null;
            b2_0 = null;
            a = 101;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A1_"+strObjectName);
        }
        void add() {
            sum = a + b1_0.a + b2_0.a;
        }
    }
    class Nocycle_a_00160_A2 {
        Nocycle_a_00160_B1 b1_0;
        Nocycle_a_00160_B2 b2_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00160_A2(String strObjectName) {
            b1_0 = null;
            b2_0 = null;
            a = 102;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A2_"+strObjectName);
        }
        void add() {
            sum = a + b1_0.a + b2_0.a;
        }
    }
    class Nocycle_a_00160_A3 {
        Nocycle_a_00160_B1 b1_0;
        Nocycle_a_00160_B2 b2_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00160_A3(String strObjectName) {
            b1_0 = null;
            b2_0 = null;
            a = 103;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A3_"+strObjectName);
        }
        void add() {
            sum = a + b1_0.a + b2_0.a;
        }
    }
    class Nocycle_a_00160_A4 {
        Nocycle_a_00160_B1 b1_0;
        Nocycle_a_00160_B2 b2_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00160_A4(String strObjectName) {
            b1_0 = null;
            b2_0 = null;
            a = 104;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A4_"+strObjectName);
        }
        void add() {
            sum = a + b1_0.a + b2_0.a;
        }
    }
    class Nocycle_a_00160_B1 {
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00160_B1(String strObjectName) {
            a = 201;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_B1_"+strObjectName);
        }
        void add() {
            sum = a + a;
        }
    }
    class Nocycle_a_00160_B2 {
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00160_B2(String strObjectName) {
            a = 202;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_B2_"+strObjectName);
        }
        void add() {
            sum = a + a;
        }
    }
}
public class Nocycle_am_00160 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        ThreadRc_00160 A1_00160 = new ThreadRc_00160();
        ThreadRc_00160 A2_00160 = new ThreadRc_00160();
        ThreadRc_00160 A3_00160 = new ThreadRc_00160();
        ThreadRc_00160 A4_00160 = new ThreadRc_00160();
        ThreadRc_00160 A5_00160 = new ThreadRc_00160();
        ThreadRc_00160 A6_00160 = new ThreadRc_00160();
        A1_00160.start();
        A2_00160.start();
        A3_00160.start();
        A4_00160.start();
        A5_00160.start();
        A6_00160.start();
        try {
            A1_00160.join();
            A2_00160.join();
            A3_00160.join();
            A4_00160.join();
            A5_00160.join();
            A6_00160.join();
        } catch (InterruptedException e) {
        }
        if (A1_00160.check() && A2_00160.check() && A3_00160.check() && A4_00160.check() && A5_00160.check() && A6_00160.check())
            System.out.println("ExpectResult");
    }
}
