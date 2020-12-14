/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FinalizerReferenceTest05
 *- @TestCaseName: FinalizerReferenceTest05
 *- @TestCaseType: Function Testing for FinalizerReference Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，软引用+FinalizerReference指向的对象内存回收正确。
 *  -#step1: 创建带有重构finalize方法的类ReferenceTest05,
 *  -#step2: 该实例的对象gc回收时，finalize方法会将其关联到一个软引用上，判断无问题。
 *  -#step3: oom触发GC，软引用无法通过get获得finalize方法不再被调用。
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerReferenceTest05.java
 *- @ExecuteClass: FinalizerReferenceTest05
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.SoftReference;
import java.util.ArrayList;

public class FinalizerReferenceTest05 {
    static SoftReference<ReferenceTest05> sr05;
    static int check = 3;

    public static void main(String[] args) {
        new ReferenceTest05(10);
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        ReferenceTest05 rt05 = sr05.get();
        if (rt05.stringArray.length == 10 && rt05.stringArray[9].equals("test9")) {
            check--;
        }
        rt05 = null;
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
        }
        Runtime.getRuntime().runFinalization();
        if (sr05.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }

    private static int oomTest() {
        int sum = 0;
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;

        for (int i = 1024 * 1024; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
}

class ReferenceTest05 {
    String[] stringArray;

    public ReferenceTest05(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }

    @Override
    public void finalize() {
        System.out.println("ExpectResult");
        FinalizerReferenceTest05.sr05 = new SoftReference(this);
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
