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


class ThreadRc_00090 extends Thread {
    private boolean checkout;
    public void run() {
        Nocycle_a_00090_A1 a1_main = new Nocycle_a_00090_A1("a1_main");
        a1_main.b1_0 = new Nocycle_a_00090_B1("b1_0");
        a1_main.b1_0.c1_0 = new Nocycle_a_00090_C1("c1_0");
        a1_main.b1_0.c1_0.d1_0 = new Nocycle_a_00090_D1("d1_0");
        a1_main.add();
        a1_main.b1_0.add();
        a1_main.b1_0.c1_0.add();
        a1_main.b1_0.c1_0.d1_0.add();
//		 System.out.printf("RC-Testing_Result=%d\n",a1_main.sum+a1_main.b1_0.sum+a1_main.b1_0.c1_0.sum+a1_main.b1_0.c1_0.d1_0.sum);
        int result = a1_main.sum + a1_main.b1_0.sum + a1_main.b1_0.c1_0.sum + a1_main.b1_0.c1_0.d1_0.sum;
        //System.out.println("RC-Testing_Result="+result);
        if (result == 2308)
            checkout = true;
        //System.out.println(checkout);
    }
    public boolean check() {
        return checkout;
    }
    class Nocycle_a_00090_A1 {
        Nocycle_a_00090_B1 b1_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00090_A1(String strObjectName) {
            b1_0 = null;
            a = 101;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_A1_"+strObjectName);
        }
        void add() {
            sum = a + b1_0.a;
        }
    }
    class Nocycle_a_00090_B1 {
        Nocycle_a_00090_C1 c1_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00090_B1(String strObjectName) {
            c1_0 = null;
            a = 201;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_B1_"+strObjectName);
        }
        void add() {
            sum = a + c1_0.a;
        }
    }
    class Nocycle_a_00090_C1 {
        Nocycle_a_00090_D1 d1_0;
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00090_C1(String strObjectName) {
            d1_0 = null;
            a = 301;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_C1_"+strObjectName);
        }
        void add() {
            sum = a + d1_0.a;
        }
    }
    class Nocycle_a_00090_D1 {
        int a;
        int sum;
        String strObjectName;
        Nocycle_a_00090_D1(String strObjectName) {
            a = 401;
            sum = 0;
            this.strObjectName = strObjectName;
//	    System.out.println("RC-Testing_Construction_D1_"+strObjectName);
        }
        void add() {
            sum = a + a;
        }
    }
}
public class Nocycle_am_00090 {
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
	Runtime.getRuntime().gc();
	rc_testcase_main_wrapper();
    }
    private static void rc_testcase_main_wrapper() {
        ThreadRc_00090 A1_00090 = new ThreadRc_00090();
        ThreadRc_00090 A2_00090 = new ThreadRc_00090();
        ThreadRc_00090 A3_00090 = new ThreadRc_00090();
        ThreadRc_00090 A4_00090 = new ThreadRc_00090();
        ThreadRc_00090 A5_00090 = new ThreadRc_00090();
        ThreadRc_00090 A6_00090 = new ThreadRc_00090();
        A1_00090.start();
        A2_00090.start();
        A3_00090.start();
        A4_00090.start();
        A5_00090.start();
        A6_00090.start();
        try {
            A1_00090.join();
            A2_00090.join();
            A3_00090.join();
            A4_00090.join();
            A5_00090.join();
            A6_00090.join();
        } catch (InterruptedException e) {
        }
        if (A1_00090.check() && A2_00090.check() && A3_00090.check() && A4_00090.check() && A5_00090.check() && A6_00090.check())
            System.out.println("ExpectResult");
    }
}
