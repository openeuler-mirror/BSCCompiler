/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest07
 *- @TestCaseName: GCOnly_WeakRefTest07
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 * -#step1：分别创建Cycle_BDec_00010_A1_Cleaner类的实例对象cycleBA1，InCycle类的实例对象cycleA；
 * -#step2：调用System.gc()进行垃圾回收；
 * -#step3：以cycleBA1为参数，执行cycleA的setCleanerCycle()方法，并记其返回值为result；
 * -#step4：创建一个Cycle_BDec_00010_A2_Cleaner的实例对象cycleBA2，并令cycleBA1.cycleBA2和cycleBA1均为null；
 * -#step5：经判断得知result为true，并令当前线程休眠2000ms，并判断得出cycleBA1.cleaner、cycleBA1.cycleBA2.cleaner、
 *          cycleBA1、cycleBA1.cycleBA2至少有一个为null；
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest07.java
 *- @ExecuteClass: GCOnly_WeakRefTest07
 *- @ExecuteArgs:
 *- @Remark:RC0349-rc-rp-simpleweakref_02
 *- @Author:zhangwenlong 00292413
 */
import sun.misc.Cleaner;

public class GCOnly_WeakRefTest07 {
    static int TEST_NUM = 1;
    static int judgeNum = 0;

    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
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
    public static boolean modifyCleanerA1(Cycle_BDec_00010_A1_Cleaner cycleBDA) {
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
        ret = modifyCleanerA1(cycleBDA1Cleaner);
        // 环正确，且reference都没释放
        if (ret == true && cycleBDA1Cleaner.cleaner == null && cycleBDA1Cleaner.cycleBA2.cleaner == null
                && cycleBDA1Cleaner != null && cycleBDA1Cleaner.cycleBA2 != null) {
            return true;
        } else {
            return false;
        }
    }
}
