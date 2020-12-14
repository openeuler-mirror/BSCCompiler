/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FinalizerReferenceTest02
 *- @TestCaseName: FinalizerReferenceTest02
 *- @TestCaseType: Function Testing for FinalizerReference Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，弱引用+FinalizerReference指向的对象
 *  -#step1: 创建带有重构finalize方法的类ReferenceTest02;
 *  -#step2: 创建一个弱引用引用指向ReferenceTest02的实例，判断无问题。
 *  -#step3: 触发用户GC，弱引用无法通过get获得，finalize方法被调用。
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerReferenceTest02.java
 *- @ExecuteClass: FinalizerReferenceTest02
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.WeakReference;

public class FinalizerReferenceTest02 {
    static int check = 2;

    public static void main(String[] args) {
        WeakReference<ReferenceTest02> weakReference = new WeakReference<>(new ReferenceTest02(10));
        ReferenceTest02 rt02 = weakReference.get();
        if (rt02.stringArray.length == 10 && rt02.stringArray[9].equals("test9")) {
            check--;
        }
        rt02 = null;
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (weakReference.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}

class ReferenceTest02 {
    String[] stringArray;

    public ReferenceTest02(int length) {
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
