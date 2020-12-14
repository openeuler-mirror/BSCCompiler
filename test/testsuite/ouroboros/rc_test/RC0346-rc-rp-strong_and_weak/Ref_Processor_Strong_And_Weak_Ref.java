/*
 *- @TestCaseID: maple/runtime/rc/function/Ref_Processor_Strong_And_Weak_Ref.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Reference Processor: strong and weak reference, first free strong, next free weak
 *
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建弱引用，软引用，虚引用，判断这些引用不为空。
 * -#step2: 将弱引用，软引用，虚引用的引用的对象置为NULL，释放强引用。
 * -#step3: 等待一个时间周期，检查引用，软引用，虚引用及对应引用队列被释放。
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: Ref_Processor_Strong_And_Weak_Ref.java
 *- @ExecuteClass: Ref_Processor_Strong_And_Weak_Ref
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.*;

public class Ref_Processor_Strong_And_Weak_Ref {
    static int check_count = 0;
    static final int TEST_NUM = 1;
    static String rf = new String("test");
    static Reference wr, sr, pr;
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

        if (check_count == 0){
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult check_count : " + check_count);
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
            check_count++;
            System.out.println("error in test01---------------wr");
        }

        sr = new WeakReference<Object>(obj2, srq);
        if (sr.get() == null) {
            check_count++;
            System.out.println("error in test01----------------sr");
        }

        pr = new PhantomReference<Object>(obj3, prq);
        if (pr.get() != null) {
            check_count++;
            System.out.println("error in test01----------------pr");
        }

        obj1 = null;
        obj2 = null;
        obj3 = null;
        if (rf == null) {
            check_count++;
            System.out.println("error in test01-----------rf");
        }
    }

    private static void checkRq(String funName) {
        Reference wrqPoll, srqPoll, prqPoll;
        sleep(2000);
        if (wr.get() != null) {
            System.out.println("error in checkRq---------------wr");
            check_count++;
        }
        if (sr.get() != null) {
            System.out.println("error in checkRq--------------sr");
            check_count++;
        }
        if (rf == null || (!rf.equals("test"))) {
            System.out.println("error in checkRq---------------rf");
            check_count++;
        }
        while ((wrqPoll = wrq.poll()) != null) {
            if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                check_count++;
                System.out.println("ErrorResult in wrq.poll()");
            }
        }
        while ((srqPoll = srq.poll()) != null) {
            if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                check_count++;
                System.out.println("ErrorResult in srq.poll()");
            }
        }
        while ((prqPoll = prq.poll()) != null) {
            if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                check_count++;
                System.out.println("ErrorResult in prq.poll()");
            }
        }
    }

    private static void sleep(int sleepNum) {
        try {
            Thread.sleep(sleepNum);
        } catch (InterruptedException e) {
            System.out.println("sleep was Interrupted");
        }
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
