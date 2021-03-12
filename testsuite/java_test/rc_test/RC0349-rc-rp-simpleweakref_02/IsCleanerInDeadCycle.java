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
public class IsCleanerInDeadCycle {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            isCleanerInDeadCycle();
            Runtime.getRuntime().gc();
            isCleanerInDeadCycle();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }
    static void isCleanerInDeadCycle() throws InterruptedException {
        judgeNum = 0;
        Cycle_BDec_00010_A1_Cleaner cycleBA1 = new Cycle_BDec_00010_A1_Cleaner();
        InCycle cycleA = new InCycle();
        System.gc();
        boolean result = cycleA.setCleanerCycle(cycleBA1);
        cycleBA1.cycleBA2 = null;
        cycleBA1 = null;
        if (result == true) {
            Thread.sleep(2000);
            if (cycleBA1.cleaner == null || cycleBA1.cycleBA2.cleaner == null) {
            } else {
                judgeNum++;
            }
        } else {
            judgeNum++;
        }
    }
}
class Cycle_BDec_00010_A1_Cleaner {
    static Cleaner cleaner;
    Cycle_BDec_00010_A2_Cleaner cycleBA2;
    int num;
    int sum;
    static int value;
    Cycle_BDec_00010_A1_Cleaner() {
        cleaner.create(cycleBA2, null);
        cycleBA2 = null;
        num = 1;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + cycleBA2.num;
    }
}
class Cycle_BDec_00010_A2_Cleaner {
    Cleaner cleaner;
    Cycle_BDec_00010_A1_Cleaner cycleBA1Cleaner;
    int num;
    int sum;
    static int value;
    Cycle_BDec_00010_A2_Cleaner() {
        cleaner.create(cycleBA1Cleaner, null);
        cycleBA1Cleaner = null;
        num = 2;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + cycleBA1Cleaner.num;
    }
}
class InCycle {
    /**
     * 确认环是正确的
     *
     * @param cycleBDA 传入的是带有Referent的类实例
     * @return true:正确；false：错误
   */


    public static boolean ModifyCleanerA1(Cycle_BDec_00010_A1_Cleaner cycleBDA) {
        cycleBDA.add();
        cycleBDA.cycleBA2.add();
        int nSum = cycleBDA.sum + cycleBDA.cycleBA2.sum;
        if (nSum == 6) {
            return true;
        } else {
            return false;
        }
    }
    /**
     * 设置一个带Cleaner的环
     *
     * @param cycleBDA1Cleaner 传入的是带有Referent的类实例
     * @return true:正确；false：错误
   */


    public static boolean setCleanerCycle(Cycle_BDec_00010_A1_Cleaner cycleBDA1Cleaner) {
        cycleBDA1Cleaner.cycleBA2 = new Cycle_BDec_00010_A2_Cleaner();
        cycleBDA1Cleaner.cycleBA2.cycleBA1Cleaner = cycleBDA1Cleaner;
        boolean ret;
        ret = ModifyCleanerA1(cycleBDA1Cleaner);
        // 环正确，且reference都没释放
        if (ret == true && cycleBDA1Cleaner.cleaner == null && cycleBDA1Cleaner.cycleBA2.cleaner == null
                && cycleBDA1Cleaner != null && cycleBDA1Cleaner.cycleBA2 != null) {
            return true;
        } else {
            return false;
        }
    }
}