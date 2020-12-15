/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FinalizerReferenceTest01
 *- @TestCaseName: FinalizerReferenceTest01
 *- @TestCaseType: Function Testing for FinalizerReference Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，软引用+FinalizerReference指向的对象内存回收正确。
 *  -#step1: 创建带有重构finalize方法的类ReferenceTest01;
 *  -#step2: 创建一个软引用指向ReferenceTest01的实例，判断无问题。
 *  -#step3: oom触发GC，软引用无法通过get获得，finalize方法被调用。
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: FinalizerReferenceTest01.java
 *- @ExecuteClass: FinalizerReferenceTest01
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.SoftReference;
import java.util.ArrayList;

public class FinalizerReferenceTest01 {
    static int check = 3;

    public static void main(String[] args) {
        SoftReference<ReferenceTest01> softReference = new SoftReference<>(new ReferenceTest01(10));
        ReferenceTest01 rt01 = softReference.get();
        if (rt01.stringArray.length == 10 && rt01.stringArray[9].equals("test9")) {
            check--;
        }
        rt01 = null;
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
        }
        Runtime.getRuntime().runFinalization();
        if (softReference.get() == null) {
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

class ReferenceTest01 {
    String[] stringArray;

    public ReferenceTest01(int length) {
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
