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


import java.lang.ref.WeakReference;
class CP_Thread_06_A1 {
    static WeakReference[] test1;
    static
    int a;
    CP_Thread_06_A1 a2_0;
    int sum;
    String strObjectName;
    CP_Thread_06_A1(String strObjectName) {
        a2_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }
    void add() {
        sum = a + a;
    }
}
class CP_Thread_06_test extends Thread {
    static String str;
    private static CP_Thread_06_A1 a1_main = null;
    private volatile static CP_Thread_06_A1 a2 = null;
    private volatile static WeakReference test1;
    private static WeakReference test2;
    private static void test_CP_Thread_06_A1(int times) {
        CP_Thread_06_A1.test1 = new WeakReference[times];
        for (int i = 0; i < times; i++) {
            str = "maple" + i;
            test1 = new WeakReference<>(str);
            CP_Thread_06_A1.test1[i] = test1;
        }
    }
    private static void test_CP_Thread_06_A2(int times) {
        CP_Thread_06_A1.test1 = new WeakReference[times];
        for (int i = 0; i < times; i++) {
            test2 = new WeakReference<>("maple" + times + i);
            CP_Thread_06_A1.test1[i] = test2;
        }
    }
    public void run() {
        a1_main = new CP_Thread_06_A1("a1_main");
        a2 = new CP_Thread_06_A1("a2_0");
        a1_main.a2_0 = a2;
        a1_main.a2_0.a2_0 = a2;
        a1_main.add();
        a1_main.a2_0.add();
        test_CP_Thread_06_A1(10000);
        test_CP_Thread_06_A2(10000);
        int result = a1_main.sum + a1_main.a2_0.sum;
        //System.out.println("RC-Testing_Result="+result);
        if (result == 404)
            System.out.println("ExpectResult");
    }
}
public class CP_Thread_06 {
    private static CP_Thread_06_A1 a1_main = null;
    private static CP_Thread_06_A1 a2 = null;
    public static void main(String[] args) {
        CP_Thread_06_test cptest1 = new CP_Thread_06_test();
        cptest1.run();
        CP_Thread_06_test cptest2 = new CP_Thread_06_test();
        cptest2.run();
    }
}
