/*
 *- @TestCaseID: GC-TaskQueue-FrequentGCTest02
 *- @TestCaseName: GC-TaskQueue-FrequentGCTest02
 *- @TestCaseType: FunctionTest
 *- @RequirementName: 添加GC队列及Runtime.GC正确性保证
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief: art/maple的system.gc()里会限制频繁调用Runtime.getRuntime.gc().验证在程序初期内存充足的情况下，system.gc()不会实际执行垃圾回收操作。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: FrequentGCTest02.java
 *- @ExecuteClass: FrequentGCTest02
 *- @ExecuteArgs:
 *- @Remark:该用例不能测试RC,只能测试GCOnly。原因是RC会直接将WeakReference清理掉。
 *- @Author:
 */


import java.lang.ref.WeakReference;

public class FrequentGCTest02 {
    static int a = 100;

    public static void main(String[] args) {
        for (int i = 0; i < 100; i++) {
            WeakReference rp = new WeakReference<Object>(new Object());
            if (rp.get() == null) {
                a++;
            }
            System.gc();
            if (rp.get() == null) {
                a++;
            }
            if (a != 100) {
                System.out.println("ErrorResult");
                return;
            }
        }
        System.out.println("ExpectResult");
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
