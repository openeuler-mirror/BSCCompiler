/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest01
 *- @TestCaseName: GCOnly_WeakRefTest01
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 *  -#step1: 创建一个弱引用，判断该引用可以通过get（）正常获得object;
 *  -#step2: 在一个线程中创建一个60个无RP关联的弱引用，再次判断步骤一里创建的弱引用仍然可以通过get()方法获得对象。
 *  -#step3: 触发用户GC，判断该引用被回收到的RP中。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest01.java
 *- @ExecuteClass: GCOnly_WeakRefTest01
 *- @ExecuteArgs:
 *- @Remark:RC0304-rc-function-Ref-WeakRefTest
 *- @Author:zhangwenlong 00292413
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

public class GCOnly_WeakRefTest01 {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int aa = 100;

    static void setWeakReference() {
        rp = new WeakReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            aa++;
        }
    }

    public static void main(String[] args) throws Exception {
        setWeakReference();
        new Thread(new TriggerRP()).start();
        Thread.sleep(3000);
        if (rp.get() == null) {
            aa++;
        }
        Runtime.getRuntime().gc();
        Reference rr;
        while ((rr = rq.poll()) != null) {
            if (!rr.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                aa++;
            }
        }

        if (aa == 100) {
            System.out.println("ExpectResult");
        }
    }

    static class TriggerRP implements Runnable {
        public void run() {
            for (int i = 0; i < 60; i++) {
                WeakReference wr = new WeakReference(new Object());
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    // do nothing
                }
            }
        }
    }
}