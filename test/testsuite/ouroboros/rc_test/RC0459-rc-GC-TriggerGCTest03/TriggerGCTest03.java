/*
 *- @TestCaseID: Maple_MemoryManagement2.0_TriggerGCTest03
 *- @TestCaseName: TriggerGCTest03
 *- @TestCaseType: Function Testing for TriggerGC Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:测试正常情况下，OOM时会触发GC进行内存回收。
 *  -#step1: 创建一个软引用，判断该引用可以通过get（）正常获得object;
 *  -#step2: 用户调用oomTest()触发GC，判断该引用被回收。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: TriggerGCTest03.java
 *- @ExecuteClass: TriggerGCTest03
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.lang.ref.SoftReference;
import java.util.ArrayList;

public class TriggerGCTest03 {
    static int check = 3;

    public static void main(String[] args) {
        SoftReference<Reference03> softReference = new SoftReference<>(new Reference03(100000));
        Reference03 rt01 = softReference.get();
        if (rt01.stringArray.length == 100000 && rt01.stringArray[9].equals("test9")) {
            check--;
        }
        rt01 = null;
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
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
    }

    private static int oomTest() {
        int sum = 0;
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;

        for (int i = 1034 * 1034; i <= 1034 * 1034 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
}

class Reference03 {
    String[] stringArray;

    public Reference03(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
