/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FinalizerReferenceTest03
 *- @TestCaseName: FinalizerReferenceTest03
 *- @TestCaseType: Function Testing for FinalizerReference Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，虚引用+FinalizerReference指向的对象
 *  -#step1: 创建带有重构finalize方法的类ReferenceTest03;
 *  -#step2: 创建一个虚引用引用指向ReferenceTest03的实例，判断无问题。
 *  -#step3: 触发用户GC，虚引用无法通过get获得，finalize方法被调用。
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerReferenceTest03.java
 *- @ExecuteClass: FinalizerReferenceTest03
 *- @ExecuteArgs:
 */

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

public class FinalizerReferenceTest03 {
    static int check = 1;

    public static void main(String[] args) {
        ReferenceQueue referenceQueue = new ReferenceQueue();
        PhantomReference<ReferenceTest03> pr = new PhantomReference<>(new ReferenceTest03(10), referenceQueue);
        if (pr.get() == null) {
            check--;
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}

class ReferenceTest03 {
    String[] stringArray;

    public ReferenceTest03(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }

    @Override
    public void finalize() {
        for (int i = 0; i < this.stringArray.length; i++) {
            stringArray[i] = null;
        }
        System.out.println("ExpectResult");
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
