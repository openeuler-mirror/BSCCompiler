/*
 *- @TestCaseID: Maple_MemoryManagement2.0_TriggerGCTest08
 *- @TestCaseName: TriggerGCTest08
 *- @TestCaseType: Function Testing for TriggerGC Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:当前初始阈值是100MB，每次已分配内存超过阈值时会将阈值乘以1.5的系数并设为新的阈值。
 *  -#step1: 通过1个for循环，分配内存到100M。
 *  -#step2:创建一个弱引用，检查它可以通过get()方法获得关联的引用对象。
 *  -#step3:再次将内存分配到100*1.5=150M
 *  -#step4:验证GC被隐式触发，步骤1中创建的弱引用被回收掉了。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: TriggerGCTest08.java
 *- @ExecuteClass: TriggerGCTest08
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class TriggerGCTest08 {
    static WeakReference observer;
    static int check = 2;
    private final static int INIT_MEMORY = 100; // MB
    private final static int SEC_MEMORY = 150; // MB
    private static ArrayList<byte[]> store = new ArrayList<byte[]>();

    public static void main(String[] args) throws Exception {
        for (int i = 0; i < INIT_MEMORY; i++) {
            byte[] temp = new byte[1024*1024];
            store.add(temp);
        }
        Thread.sleep(2000);
        observer = new WeakReference<Object>(new Object());
        if (observer.get() != null) {
            check--;
        }
        for (int i = INIT_MEMORY; i < SEC_MEMORY; i++) {
            byte[] temp = new byte[1024*1024];
            store.add(temp);
        }
        Thread.sleep(3000);
        if (observer.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, check should be 0, but now check = " + check);
        }
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
