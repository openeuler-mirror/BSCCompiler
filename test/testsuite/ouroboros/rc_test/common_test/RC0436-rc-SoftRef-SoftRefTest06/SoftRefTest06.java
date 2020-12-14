import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
import java.util.ArrayList;

/*
 * -@TestCaseID:SoftRefTest06.java
 * -@TestCaseName:MyselfClassName
 * -@RequirementName:[运行时需求]支持自动内存管理
 * -@Title/Destination: SoftReference指向的对象在回收的时候，会触发自己的finalize方法。
 * -@Condition: no
 * -#c1
 * -@Brief:functionTest
 * -#step1：SoftReference指向一个软引用，该对象声明周期结束后，会立刻被回收；
 * -#step2：执行一次后，进行一遍gc，进行环的自学习；
 * -#step3：再执行一次后，判断环关联的软对象不应该被释放。
 * -@Expect:ExpectResult1\nExpectResult2\n
 * -@Priority: High
 * -@Source: SoftRefTest06.java
 * -@ExecuteClass: SoftRefTest06
 * -@ExecuteArgs:
 * -@Remark:
 *
 */

public class SoftRefTest06 {
    private static Reference rp;
    private static ReferenceQueue rq = new ReferenceQueue();
    private static ArrayList<byte[]> store;
    private static int count = 1;

    // 生命周期结束的软指针指向的对象，应该被释放，放到它的ReferenceQueue里
    public static void objectDeadNormally() {
        Reference sr = new SoftReference<SoftRefTest06>(new SoftRefTest06(), rq);
        if (sr.get() == null) {
            System.out.println("ErrorResult");
        }
        // sr will die now.
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

    private static void setSoftRef() {
        // oom出发，回收软引用对象使用的内存
        rp = new SoftReference<SoftRefTest06>(new SoftRefTest06(), rq);
    }

    public static void main(String[] args) {
        objectDeadNormally();
        Runtime.getRuntime().runFinalization();
        setSoftRef();
        try {
            int oom = oomTest();
        } catch (OutOfMemoryError ofm) {
            // do nothing
        }
        if (rp.get() != null) {
            System.out.println("Error Result when second check ");
        }
        Runtime.getRuntime().runFinalization();
    }

    @Override
    public void finalize() {
        System.out.println("ExpectResult" + count);
        count++;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult1\nExpectResult2\n
