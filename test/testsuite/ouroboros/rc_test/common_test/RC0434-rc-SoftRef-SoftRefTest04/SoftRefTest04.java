/*
 * -@TestCaseID:SoftRefTest04.java
 * -@TestCaseName:MyselfClassName
 * -@RequirementName:[运行时需求]支持自动内存管理
 * -@Title/Destination: 通过OOM 触发gc时，SoftReference对象会被回收掉
 * -@Condition: no
 * -#c1
 * -@Brief:functionTest
 * -#step1：创建一个SoftReference对象，并将其放到ReferneceQueue中，该对象不应该被释放；
 * -#step2：通过制造OOM进行GC
 * -#step3：再次确认该对象应该被释放，且它所在的ReferenceQueue中有这个即将回收的Reference对象
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: SoftRefTest04.java
 * -@ExecuteClass: SoftRefTest04
 * -@ExecuteArgs:
 * -@Remark:
 *
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
import java.util.ArrayList;

public class SoftRefTest04 {
    private static Reference rp;
    private static ReferenceQueue rq = new ReferenceQueue();
    private static int a = 100;
    private static ArrayList<byte[]> store;

    static void setSoftReference() {
        rp = new SoftReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            System.out.println("Error Result when first check ");
            a++;
        }
    }

    private static int oomTest() {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1024 * 1024; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }

    public static void main(String[] args) throws Exception {
        setSoftReference();
        try {
            int Result = oomTest();
        } catch (OutOfMemoryError o) {
            // do nothing
        }
        if (rp.get() != null) {
            System.out.println("Error Result when second check ");
            a++;
        }
        Thread.sleep(500);
        Reference r = rq.poll();
        if (r == null) {
            System.out.println("Error Result when checking ReferenceQueue");
            a++;
        }
        if (a == 100) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult finally");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
