/*
 *- @TestCaseID: Maple_MemoryManagement2.0_GCOnly_WeakRefTest06
 *- @TestCaseName: GCOnly_WeakRefTest06
 *- @TestCaseType: Function Testing for WeakReference Test
 *- @RequirementID: AR000D0OR9
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用内存回收正确。
 * -#step1：创建一个ReferenceQueue类的实例对象rq，并已null和rq为参数创建一个WeakReference的实例对象weakRp；
 * -#step2：新建一个ThreadRf线程，并开启此线程；
 * -#step3：gc；
 * -#step4：经判断得出weakRp.get()的返回值为null；
 * -#step5：判断rq.poll()的返回值为reference，直至全部判断完成；
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: GCOnly_WeakRefTest06.java
 *- @ExecuteClass: GCOnly_WeakRefTest06
 *- @ExecuteArgs:
 *- @Remark:RC0348-rc-rp-simpleweakref_01
 *- @Author:zhangwenlong 00292413
 */
import java.lang.ref.*;

public class GCOnly_WeakRefTest06 {
    static int testNum = 1;
    static int judgeNum = 0;
    static Reference weakRp;
    static ReferenceQueue rq = new ReferenceQueue();

    public static void main(String[] args) throws Exception {
        judgeNum = 0;
        for (int i = 0; i < testNum; i++) {
            deadReferenceTest();
        }
        if (judgeNum == 0) {
            System.out.println("ExpectResult");
        }
    }

    static void setWeakRef() {
        weakRp = new WeakReference<Object>(null, rq);
    }

    static void deadReferenceTest() throws InterruptedException {
        Reference reference;

        setWeakRef();
        new Thread(new ThreadRf()).start();
        Runtime.getRuntime().gc();
        if (weakRp.get() != null) {
            judgeNum++;
        }
        while ((reference = rq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                judgeNum++;
            }
        }
    }
}

class ThreadRf implements Runnable {
    public void run() {
        for (int i = 0; i < 60; i++) {
            WeakReference wr = new WeakReference(new Object());
            try {
                Thread.sleep(100);
            } catch (Exception e) {
                System.out.println(" sleep was Interrupted");
            }
        }
    }
}
