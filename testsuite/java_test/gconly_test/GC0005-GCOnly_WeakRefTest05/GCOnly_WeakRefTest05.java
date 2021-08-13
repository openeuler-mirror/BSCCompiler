/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest05
 *- @TestCaseName: GCOnly_WeakRefTest05
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 * -#step1: 创建弱引用，虚引用，判断这些引用不为空。
 * -#step2: 将弱引用，虚引用的引用的对象置为NULL，释放强引用。
 * -#step3: GC后，检查引用，软引用，虚引用及对应引用队列被释放。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest05.java
 *- @ExecuteClass: GCOnly_WeakRefTest05
 *- @ExecuteArgs:
 *- @Remark:RC0346-rc-rp-strong_and_weak
 *- @Author:zhangwenlong 00292413
 */

import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class GCOnly_WeakRefTest05 {
    static final int TEST_NUM = 1;
    static int checkCount = 0;
    static String rf = "test";
    static Reference wr, pr;
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static StringBuffer obj1 = new StringBuffer("weak");
    static StringBuffer obj2 = new StringBuffer("soft");
    static StringBuffer obj3 = new StringBuffer("phantom");

    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();

        if (checkCount == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult checkCount : " + checkCount);
        }
    }

    private static void test() {
        // strong and weak reference, first free strong, next free weak
        for (int i = 0; i < TEST_NUM; i++) {
            test_01();
        }
        checkRq("test_01_02");
    }

    private static void test_01() {
        /* strong and weak reference, first free strong, next free weak */
        obj1 = new StringBuffer("weak");
        obj2 = new StringBuffer("soft");
        obj3 = new StringBuffer("phantom");
        wr = new WeakReference<Object>(obj1, wrq);
        if (wr.get() == null) {
            checkCount++;
            System.out.println("error in test01---------------wr");
        }

        pr = new PhantomReference<Object>(obj3, prq);
        if (pr.get() != null) {
            checkCount++;
            System.out.println("error in test01----------------pr");
        }

        obj1 = null;
        obj2 = null;
        obj3 = null;
        if (rf == null) {
            checkCount++;
            System.out.println("error in test01-----------rf");
        }
    }

    private static void checkRq(String funName) {
        Reference wrqPoll, srqPoll, prqPoll;
        Runtime.getRuntime().gc();
        if (wr.get() != null) {
            System.out.println("error in checkRq---------------wr");
            checkCount++;
        }
        if (rf == null || (!rf.equals("test"))) {
            System.out.println("error in checkRq---------------rf");
            checkCount++;
        }
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println("ErrorResult in wrq.poll()");
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                checkCount++;
                System.out.println("ErrorResult in srq.poll()");
            }
        }
        while ((prqPoll = prq.poll()) != null) {
            if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                checkCount++;
                System.out.println("ErrorResult in prq.poll()");
            }
        }
    }
}


