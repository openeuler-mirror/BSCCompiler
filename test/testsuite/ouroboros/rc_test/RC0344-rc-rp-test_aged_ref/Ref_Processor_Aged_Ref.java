/*
 *- @TestCaseID:maple/runtime/rc/function/Ref_Processor_Aged_Ref.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Reference Processor: 1. Threshold for Ref object == age - 1
 *                                           2. Threshold for Ref object == age
 *                                           3. Threshold for Ref object == age + 1
 *                                           4. Has entered the Aged Ref queue, the number of processing == X-1
 *                                           5. Has entered the Aged Ref queue, the number of processing == X
 *                                           6. Has entered the Aged Ref queue, the number of processing == X+1
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 创建软引用，弱引用，虚引用，并判断创建后引用不为空。
 * -#step2: 等待一个时间，引用应该被回收为空，引用队列也为空。
 * -#step3: 将步骤1,2的操作分别重复age-1、age、age+1。
 * -#step4: 此时会到达age的引用队列，重复步骤1~3。
 * -#step5: 调用Runtime.getRuntime().gc()进行资源回收。
 * -#step6: 重复步骤1~4。
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: Ref_Processor_Aged_Ref.java
 *- @ExecuteClass: Ref_Processor_Aged_Ref
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.*;

public class Ref_Processor_Aged_Ref {
    static int check_count = 0;
    static final int TEST_NUM = 1;
    static final int AGED_NUM = 3;
    static WeakReference sr[] = new WeakReference[AGED_NUM + 1];
    static WeakReference wr[] = new WeakReference[AGED_NUM + 1];
    static PhantomReference pr[] = new PhantomReference[AGED_NUM + 1];
    static ReferenceQueue wrq = new ReferenceQueue();
    static ReferenceQueue srq = new ReferenceQueue();
    static ReferenceQueue prq = new ReferenceQueue();
    static StringBuffer obj1 = new StringBuffer("WeakReference");
    static StringBuffer obj2 = new StringBuffer("WeakReference");
    static StringBuffer obj3 = new StringBuffer("PhantomReference");

    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();

        if (check_count == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, check_count : " + check_count);
        }
    }

    public static void test() {
        for (int i = 0; i < TEST_NUM; i++) {
            // Threshold for Ref object == age - 1
            obj1 = new StringBuffer("WeakReference");
            obj2 = new StringBuffer("WeakReference");
            obj3 = new StringBuffer("PhantomReference");
            setRef(AGED_NUM - 1);
            checkRef(AGED_NUM - 1);
            // Threshold for Ref object == age
            obj1 = new StringBuffer("WeakReference");
            obj2 = new StringBuffer("WeakReference");
            obj3 = new StringBuffer("PhantomReference");
            setRef(AGED_NUM);
            checkRef(AGED_NUM);
            // Threshold for Ref object == age + 1
            obj1 = new StringBuffer("WeakReference");
            obj2 = new StringBuffer("WeakReference");
            obj3 = new StringBuffer("PhantomReference");
            setRef(AGED_NUM + 1);
            checkRef(AGED_NUM + 1);
            // Has entered the Aged Ref queue
            // The number of processing == X-1
            obj1 = new StringBuffer("WeakReference");
            obj2 = new StringBuffer("WeakReference");
            obj3 = new StringBuffer("PhantomReference");
            setRef(AGED_NUM - 1);
            checkRef(AGED_NUM - 1);
            //the number of processing == X
            obj1 = new StringBuffer("WeakReference");
            obj2 = new StringBuffer("WeakReference");
            obj3 = new StringBuffer("PhantomReference");
            setRef(AGED_NUM);
            checkRef(AGED_NUM);
            //the number of processing == X+1
            obj1 = new StringBuffer("WeakReference");
            obj2 = new StringBuffer("WeakReference");
            obj3 = new StringBuffer("PhantomReference");
            setRef(AGED_NUM + 1);
            checkRef(AGED_NUM + 1);
        }
    }

    public static void setRef(int repeat_time) {
        /*Threshold for Ref object == age - 1*/
        for (int i = 0; i < repeat_time; i++) {
            sr[i] = new WeakReference<StringBuffer>(obj1, srq);
            if (sr[i].get() == null) {
                check_count++;
                System.out.println("sr in setRef    " + i + "repeat_time:" + repeat_time);
            }
            wr[i] = new WeakReference<StringBuffer>(obj2, wrq);
            if (wr[i].get() == null) {
                check_count++;
                System.out.println("wr in setRef     " + i + "repeat_time:" + repeat_time);
            }
            pr[i] = new PhantomReference<StringBuffer>(obj3, prq);
            if (pr[i].get() != null) {
                check_count++;
                System.out.println("pr in setRef");
            }
        }
        obj1 = null;
        obj2 = null;
        obj3 = null;
    }

    public static void checkRef(int repeat_time) {
        sleep(2000);
        for (int i = 0; i < repeat_time; i++) {
            if (sr[i].get() != null) {
                check_count++;
            }
            if (wr[i].get() != null) {
                check_count++;
            }
            Reference wrqPoll, srqPoll, prqPoll;
            while ((wrqPoll = wrq.poll()) != null) {
                if (!wrqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in wrq.poll()");
                }
            }
            while ((srqPoll = srq.poll()) != null) {
                if (!srqPoll.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in srq.poll()");
                }
            }
            while ((prqPoll = prq.poll()) != null) {
                if (!prqPoll.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                    check_count++;
                    System.out.println(" ErrorResult in prq.poll()");
                }
            }
        }
    }

    private static void sleep(int sleepNum) {
        try {
            Thread.sleep(sleepNum);
        } catch (InterruptedException e) {
            System.out.println(" sleep was Interrupted");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
