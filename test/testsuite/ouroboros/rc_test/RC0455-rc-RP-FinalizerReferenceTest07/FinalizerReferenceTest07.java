/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FinalizerReferenceTest07
 *- @TestCaseName: FinalizerReferenceTest07
 *- @TestCaseType: Function Testing for FinalizerReference Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Conditon:
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，虚引用+FinalizerReference指向的对象内存回收正确。
 *  -#step1: 创建带有重构finalize方法的类ReferenceTest07,
 *  -#step2: 该实例的对象gc回收时，finalize方法会将其关联到一个虚引用上，判断无问题。
 *  -#step3: 触发用户GC，虚引用无法通过get获得finalize方法不再被调用。
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerReferenceTest07.java
 *- @ExecuteClass: FinalizerReferenceTest07
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

public class FinalizerReferenceTest07 {
    static ReferenceQueue referenceQueue = new ReferenceQueue();
    static PhantomReference<ReferenceTest07> sr07;
    static int check = 2;

    public static void main(String[] args) {
        new ReferenceTest07(10);
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (sr07.get() == null) {
            check--;
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // do nothing
        }
        if (referenceQueue.poll() != null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}

class ReferenceTest07 {
    String[] stringArray;

    public ReferenceTest07(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }

    @Override
    public void finalize() {
        System.out.println("ExpectResult");
        FinalizerReferenceTest07.sr07 = new PhantomReference<>(this, FinalizerReferenceTest07.referenceQueue);
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
