/*
 *- @TestCaseID:SoftRefTest02.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: 正常情况下通过system.gc()强制gc时，SoftReference对象不会被回收掉
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1：创建一个SoftReference对象，并将其放到ReferneceQueue中，该对象不应该被释放；
 * -#step2：通过System.gc()进行强制GC
 * -#step3：再次确认该对象不应该被释放，且它所在的ReferenceQueue中没有即将回收的Reference对象
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: SoftRefTest02.java
 *- @ExecuteClass: SoftRefTest02
 *- @ExecuteArgs:
 *- @Remark:
 *
 */

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;

public class SoftRefTest02 {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int a = 100;

    static void setSoftReference() {
        rp = new SoftReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            System.out.println("Error Result when first check ");
            a++;
        }
    }

    public static void main(String[] args) throws Exception {
        setSoftReference();
        for (int i = 0; i < 3; i++) {
            System.gc();
            if (rp.get() == null) {
                System.out.println("Error Result when second check ");
                a++;
            }
        }
        Reference r = rq.poll();
        if (r != null) {
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
