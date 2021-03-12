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


import com.huawei.ark.annotation.Weak;
import com.huawei.ark.annotation.Unowned;
class RCAnnotationThread02_1 extends Thread {
    public void run() {
        RCAnnotationThread02 rcth01 = new RCAnnotationThread02();
        try {
            rcth01.setA1null();
        } catch (NullPointerException e) {
        }
    }
}
class RCAnnotationThread02_2 extends Thread {
    public void run() {
        RCAnnotationThread02 rcth01 = new RCAnnotationThread02();
        try {
            rcth01.setA4null();
        } catch (NullPointerException e) {
        }
    }
}
class RCAnnotationThread02_3 extends Thread {
    public void run() {
        RCAnnotationThread02 rcth01 = new RCAnnotationThread02();
        try {
            rcth01.setA5null();
        } catch (NullPointerException e) {
        }
    }
}
public class RCAnnotationThread02 {
    private volatile  static RCAnnotationThread02_A1 a1_main = null;
    private volatile  static  RCAnnotationThread02_A4 a4_main = null;
    private volatile  static  RCAnnotationThread02_A5 a5_main = null;
    RCAnnotationThread02() {
        synchronized (this) {
            try {
                RCAnnotationThread02_A1 a1 = new RCAnnotationThread02_A1();
                a1.a2_0 = new RCAnnotationThread02_A2();
                a1.a2_0.a3_0 = new RCAnnotationThread02_A3();
                RCAnnotationThread02_A4 a4 = new RCAnnotationThread02_A4();
                RCAnnotationThread02_A5 a5 = new RCAnnotationThread02_A5();
                a4.a1_0 = a1;
                a5.a1_0 = a1;
                a1.a2_0.a3_0.a1_0 = a1;
                a1_main = a1;
                a4_main = a4;
                a5_main = a5;
            } catch (NullPointerException e) {
            }
        }
    }
    public static void main(String[] args) {
        rc_testcase_main_wrapper();
        System.out.println("ExpectResult");
    }
    private static void rc_testcase_main_wrapper() {
        RCAnnotationThread02_1 t1=new RCAnnotationThread02_1();
        RCAnnotationThread02_2 t2=new RCAnnotationThread02_2();
        RCAnnotationThread02_3 t3=new RCAnnotationThread02_3();
        t1.start();
        t2.start();
        t3.start();
        try{
            t1.join();
            t2.join();
            t3.join();
        }catch(InterruptedException e){}
    }
    public void setA1null() {
        a1_main = null;
    }
    public void setA4null() {
        a4_main = null;
    }
    public void setA5null() {
        a5_main = null;
    }
    static class RCAnnotationThread02_A1 {
        volatile RCAnnotationThread02_A2 a2_0;
        int a;
        int sum;
        RCAnnotationThread02_A1() {
            a2_0 = null;
            a = 1;
            sum = 0;
        }
        void add() {
            sum = a + a2_0.a;
        }
    }
    static class RCAnnotationThread02_A2 {
        volatile RCAnnotationThread02_A3 a3_0;
        int a;
        int sum;
        RCAnnotationThread02_A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
        void add() {
            sum = a + a3_0.a;
        }
    }
    static class RCAnnotationThread02_A3 {
        @Weak
        volatile RCAnnotationThread02_A1 a1_0;
        int a;
        int sum;
        RCAnnotationThread02_A3() {
            a1_0 = null;
            a = 3;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a;
        }
    }
    static class RCAnnotationThread02_A4 {
        @Unowned
        volatile RCAnnotationThread02_A1 a1_0;
        int a;
        int sum;
        RCAnnotationThread02_A4() {
            a1_0 = null;
            a = 4;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a;
        }
    }
    static class RCAnnotationThread02_A5 {
        @Unowned
        volatile RCAnnotationThread02_A1 a1_0;
        int a;
        int sum;
        RCAnnotationThread02_A5() {
            a1_0 = null;
            a = 5;
            sum = 0;
        }
        void add() {
            sum = a + a1_0.a;
        }
    }
}
