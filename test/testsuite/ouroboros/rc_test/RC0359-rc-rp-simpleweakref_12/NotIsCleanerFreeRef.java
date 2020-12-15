/*
 *- @TestCaseID: RefProCase/RefProcessor/src/NotIsCleanerFreeRef.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: RefProcessor testcase: 不是cleaner对象，并且释放对象
 *- @Condition: no
 *- @Brief:functionTest
 * -#step1: 构造一个弱引用, 确认引用不为空。
 * -#step2: 等待一个周期，确认弱引用被释放
 *- @Expect: ExpectResult\n
 *- @Priority: High
 *- @Source: NotIsCleanerFreeRef.java
 *- @ExecuteClass: NotIsCleanerFreeRef
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class NotIsCleanerFreeRef {
    static int TEST_NUM = 1;
    static int judgeNum = 0;
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer obj = new StringBuffer("soft");

    static void setSoftRef() {
        obj = new StringBuffer("soft");
        rp = new WeakReference<Object>(obj, rq);
        if (rp.get() == null) {
            judgeNum++;
        }
        obj = null;
    }

    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < TEST_NUM; i++) {
            notIsCleanerFreeRef();
            Runtime.getRuntime().gc();
            notIsCleanerFreeRef();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }

    static void notIsCleanerFreeRef() throws InterruptedException {
        Reference ref;
        setSoftRef();

        new Thread(new ThreadRef()).start();
        Thread.sleep(2000);
        if (rp.get() != null) {
            judgeNum++;
        }
        while ((ref = rq.poll()) != null) {
            if (!ref.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                judgeNum++;
            }
        }
    }
}

class ThreadRef implements Runnable {
    public void run() {
        for (int i = 0; i < 60; i++) {
            WeakReference wr = new WeakReference(new Object());
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                System.out.println(" sleep was Interrupted");
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
