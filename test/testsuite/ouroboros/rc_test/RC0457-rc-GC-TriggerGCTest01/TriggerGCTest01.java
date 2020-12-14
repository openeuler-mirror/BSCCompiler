/*
 *- @TestCaseID: Maple_MemoryManagement2.0_TriggerGCTest01
 *- @TestCaseName: TriggerGCTest01
 *- @TestCaseType: Function Testing for TriggerGC Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，用户通过System.gc()进行内存回收时处理正确。
 *  -#step1: 创建一个弱引用，判断该引用可以通过get（）正常获得object;
 *  -#step2: 在一个线程中创建一个60个无RP关联的弱引用，再次判断步骤一里创建的弱引用仍然可以通过get()方法获得对象。
 *  -#step3: 用户调用System.gc()触发GC，判断该引用被回收到的RP中。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: TriggerGCTest01.java
 *- @ExecuteClass: TriggerGCTest01
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class TriggerGCTest01 {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int a = 100;

    static void setWeakReference() {
        rp = new WeakReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            a++;
        }
    }

    public static void main(String[] args) throws Exception {
        Reference r;
        setWeakReference();
        new Thread(new TriggerRP()).start();
        Runtime.getRuntime().gc();
        Thread.sleep(1000);
        if (rp.get() != null) {
            a++;
        }
        if (a == 100) {
            System.out.println("ExpectResult");
        }
    }

    static class TriggerRP implements Runnable {
        public void run() {
            for (int i = 0; i < 60; i++) {
                WeakReference wr = new WeakReference(new Object());
                try {
                    Thread.sleep(50);
                } catch (Exception e) {
                }
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
