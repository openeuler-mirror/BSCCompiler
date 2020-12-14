/*
 *- @TestCaseID: GC-TaskQueue-FrequentGCTest01
 *- @TestCaseName: GC-TaskQueue-FrequentGCTest01
 *- @TestCaseType: FunctionTest
 *- @RequirementName: 添加GC队列及Runtime.GC正确性保证
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief: 验证频繁调用Runtime.getRuntime.gc()，GC队列仍然能保证GC的正确性。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: FrequentGCTest01.java
 *- @ExecuteClass: FrequentGCTest01
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:
 */


import java.lang.ref.WeakReference;

public class FrequentGCTest01 {
    static int a = 100;

    public static void main(String[] args) throws Exception {
        for (int i = 0; i < 100; i++) {
            WeakReference rp = new WeakReference<Object>(new Object());
            if (rp.get() == null) {
                a++;
            }
            new Thread(new TriggerRP()).start();
            Runtime.getRuntime().gc();
            Thread.sleep(100);
            if (rp.get() != null) {
                a++;
            }
            if (a != 100) {
                System.out.println("ErrorResult");
                return;
            }
        }
        System.out.println("ExpectResult");
    }

    static class TriggerRP implements Runnable {
        public void run() {
            for (int i = 0; i < 10; i++) {
                WeakReference wr = new WeakReference(new Object());
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
