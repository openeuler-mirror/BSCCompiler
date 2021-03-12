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


import java.util.ArrayList;
import com.huawei.ark.annotation.Permanent;
import java.lang.ref.WeakReference;
public class RCPermanentTest4 {
    static {
        System.loadLibrary("jniTestHelper");
    }
    public static native boolean isHeapObject(Object obj);
    public static native int refCount(Object obj);
    public static int checkNum = 0;
    static Object owner;
    static boolean checkRC(Object obj) {
        int rc1 = refCount(obj);
        owner = obj;
        int rc2 = refCount(obj);
        owner = null;
        int rc3 = refCount(obj);
        if (rc1 != rc3) {
            throw new RuntimeException("rc incorrect!");
        }
        if (rc1 == rc2 && rc2 == rc3) {
            //如果相等，说明annotation生效，没有经过RC处理
            return false;
        } else {
            return true;
        }
    }
    /*
      验证环场景
     */


    static void method1() {
        Cycle_A a = new Cycle_A();
        a.b = null;
        a.b = new @Permanent Cycle_B();
        a.b.a = a;
        int result = a.add() + a.b.add();
        if (result == 16) {
            checkNum++;
        } else {
            System.out.println("result:" + result);
        }
        boolean resultA1 = checkRC(a);
        boolean resultA2 = isHeapObject(a);
        boolean resultB1 = checkRC(a.b);
        boolean resultB2 = isHeapObject(a.b);
        if (String.valueOf(resultA1).equals("true") && String.valueOf(resultA2).equals("true") && String.valueOf(resultB1).equals("false") && String.valueOf(resultB2).equals("true")) {
            //if (result1 == false && result2 == true) {
            checkNum++;
        } else {
            System.out.println("error");
            System.out.println("CycleA_checkRC:" + resultA1);
            System.out.println("CycleA_isHeap:" + resultA2);
            System.out.println("CycleB_checkRC:" + resultB1);
            System.out.println("CycleB_isHeap:" + resultB2);
            //System.out.println("result1:" + result1 + "       is Heap:" + result2);
        }
    }
    public static void main(String[] args) {
        method1();
        if (checkNum == 2) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("checkNum:" + checkNum);
        }
    }
}
class Cycle_A {
    Cycle_B b;
    static int sum;
    static int a;
    Cycle_A() {
        a = 3;
    }
    int add() {
        sum = a + b.b;
        return sum;
    }
}
class Cycle_B {
    Cycle_A a;
    static int sum;
    static int b;
    Cycle_B() {
        b = 5;
    }
    int add() {
        sum = a.a + b;
        return sum;
    }
}
