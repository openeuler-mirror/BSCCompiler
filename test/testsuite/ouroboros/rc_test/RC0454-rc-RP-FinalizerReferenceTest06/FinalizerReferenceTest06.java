/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FinalizerReferenceTest06
 *- @TestCaseName: FinalizerReferenceTest06
 *- @TestCaseType: Function Testing for FinalizerReference Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用+FinalizerReference指向的对象内存回收正确。
 *  -#step1: 创建带有重构finalize方法的类ReferenceTest06,
 *  -#step2: 该实例的对象gc回收时，finalize方法会将其关联到一个弱引用上，判断无问题。
 *  -#step3: 触发用户GC，弱引用无法通过get获得finalize方法不再被调用。
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerReferenceTest06.java
 *- @ExecuteClass: FinalizerReferenceTest06
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.WeakReference;

public class FinalizerReferenceTest06 {
    static WeakReference<ReferenceTest06> sr06;
    static int check = 2;

    public static void main(String[] args) {
        new ReferenceTest06(10);
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (sr06.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}

class ReferenceTest06 {
    String[] stringArray;

    public ReferenceTest06(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }

    @Override
    public void finalize() {
        System.out.println("ExpectResult");
        FinalizerReferenceTest06.sr06 = new WeakReference<>(this);
        ReferenceTest06 rt06 = FinalizerReferenceTest06.sr06.get();
        if (rt06.stringArray.length == 10 && rt06.stringArray[9].equals("test9")) {
            FinalizerReferenceTest06.check--;
        }
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
