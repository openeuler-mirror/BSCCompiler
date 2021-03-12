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


import sun.misc.Cleaner;
import java.lang.ref.*;
public class IsCleanerNotInDeadCycleNotSetWCBFailAtomic {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            isCleanerNotInDeadCycleNotSetWCBFailAtomic();
            Runtime.getRuntime().gc();
            isCleanerNotInDeadCycleNotSetWCBFailAtomic();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }
    static void isCleanerNotInDeadCycleNotSetWCBFailAtomic() throws InterruptedException {
        Cycle_BDec_00010_A1_Cleaner cleanerClass1 = new Cycle_BDec_00010_A1_Cleaner();
        InCycle cycleA = new InCycle();
        cycleA.setCleanerCycle(cleanerClass1);
        System.gc();
        boolean result = cycleA.setCleanerCycle(cleanerClass1);
        if (result == false) {
            judgeNum++;
        }
    }
}
class Cycle_BDec_00010_A1_Cleaner {
    static Cleaner cleaner;
    static ReferenceQueue rq = new ReferenceQueue();
    Cycle_BDec_00010_A2_Cleaner cleanerClass2;
    int num;
    int sum;
    static int value;
    Cycle_BDec_00010_A1_Cleaner() {
        cleaner.create(cleanerClass2, null);
        cleanerClass2 = null;
        num = 1;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + cleanerClass2.num;
    }
}
class Cycle_BDec_00010_A2_Cleaner {
    Cleaner cleaner;
    static ReferenceQueue rq = new ReferenceQueue();
    Cycle_BDec_00010_A1_Cleaner cleanerClass1;
    int num;
    int sum;
    static int value;
    Cycle_BDec_00010_A2_Cleaner() {
        cleaner.create(cleanerClass1, null);
        cleanerClass1 = null;
        num = 2;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + cleanerClass1.num;
    }
}
class InCycle {
    /**
     * 确认环是正确的
     *
     * @param cleaner 传入的是带有Referent的类实例
     * @return 返回布尔值，判断运算的结果正确
   */


    public static boolean ModifyCleanerA1(Cycle_BDec_00010_A1_Cleaner cleaner) {
        cleaner.add();
        cleaner.cleanerClass2.add();
        int nSum = cleaner.sum + cleaner.cleanerClass2.sum;
        if (nSum == 6) {
            return true;
        } else {
            return false;
        }
    }
    /**
     * 设置一个带Cleaner的环
     *
     * @param cleaner 传入的是带有Referent的类实例
     * @return 返回布尔值，判断reference没释放
   */


    public static boolean setCleanerCycle(Cycle_BDec_00010_A1_Cleaner cleaner) {
        cleaner.cleanerClass2 = new Cycle_BDec_00010_A2_Cleaner();
        cleaner.cleanerClass2.cleanerClass1 = cleaner;
        boolean ret;
        ret = ModifyCleanerA1(cleaner);
        System.gc();
        ret = ModifyCleanerA1(cleaner);
        // 环正确，且reference都没释放
        if (ret == true && cleaner.cleaner == null && cleaner.cleanerClass2.cleaner == null && cleaner != null
                && cleaner.cleanerClass2 != null) {
            return true;
        } else {
            return false;
        }
    }
}