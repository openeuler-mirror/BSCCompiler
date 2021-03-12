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
public class IsCleanerNotInDeadCycleFreeRef {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            isCleanerNotInDeadCycleFreeRef();
            Runtime.getRuntime().gc();
            isCleanerNotInDeadCycleFreeRef();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }
    static void isCleanerNotInDeadCycleFreeRef() throws InterruptedException {
        Cleaner cleaner;
        Cycle_BDec_00010_A1_Cleaner cycleBDA1Cleaner = new Cycle_BDec_00010_A1_Cleaner();
        InCycle cycleA = new InCycle();
        System.gc();
        boolean result = cycleA.setCleanerCycle(cycleBDA1Cleaner);
        cycleBDA1Cleaner.cycleBDA2Cleaner = null;
        if (result == true) {
            Thread.sleep(2000);
            if ((cycleBDA1Cleaner.cleaner == null) || (cycleBDA1Cleaner.cycleBDA2Cleaner.cleaner == null)
                    || (cycleBDA1Cleaner != null)) {
                while ((cleaner = (Cleaner) cycleBDA1Cleaner.rq.poll()) != null) {
                    if (!cleaner.getClass().toString().equals("class sun.misc.Cleaner")) {
                        judgeNum++;
                    }
                }
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
    static ReferenceQueue rq = new ReferenceQueue();
    Cycle_BDec_00010_A2_Cleaner cycleBDA2Cleaner;
    int num;
    int sum;
    static int value;
    Cycle_BDec_00010_A1_Cleaner() {
        cleaner.create(cycleBDA2Cleaner, null);
        cycleBDA2Cleaner = null;
        num = 1;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + cycleBDA2Cleaner.num;
    }
}
class Cycle_BDec_00010_A2_Cleaner {
    Cleaner cleaner;
    Cycle_BDec_00010_A1_Cleaner cycleBDA1;
    int num;
    int sum;
    static int value;
    Cycle_BDec_00010_A2_Cleaner() {
        cleaner.create(cycleBDA1, null);
        cycleBDA1 = null;
        num = 2;
        sum = 0;
        value = 100;
    }
    void add() {
        sum = num + cycleBDA1.num;
    }
}
class InCycle {
    /**
     * 确认环是正确的
     *
     * @param cycleBD 传入的是带有Referent的类实例
     * @return 传入的是带有Referent的类实例
   */


    public static boolean ModifyCleanerA1(Cycle_BDec_00010_A1_Cleaner cycleBD) {
        cycleBD.add();
        cycleBD.cycleBDA2Cleaner.add();
        int nSum = cycleBD.sum + cycleBD.cycleBDA2Cleaner.sum;
        if (nSum == 6) {
            return true;
        } else {
            return false;
        }
    }
    /**
     * 设置一个带Cleaner的环
     *
     * @param cycleB
     * @return
   */


    public static boolean setCleanerCycle(Cycle_BDec_00010_A1_Cleaner cycleB) {
        cycleB.cycleBDA2Cleaner = new Cycle_BDec_00010_A2_Cleaner();
        cycleB.cycleBDA2Cleaner.cycleBDA1 = cycleB;
        boolean ret;
        ret = ModifyCleanerA1(cycleB);
        // 环正确，且reference都没释放
        if (ret == true && cycleB.cleaner == null && cycleB.cycleBDA2Cleaner.cleaner == null && cycleB != null
                && cycleB.cycleBDA2Cleaner != null) {
            return true;
        } else {
            return false;
        }
    }
}