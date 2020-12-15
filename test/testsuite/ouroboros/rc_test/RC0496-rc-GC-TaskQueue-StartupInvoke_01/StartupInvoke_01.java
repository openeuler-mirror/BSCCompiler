/*
 *- @TestCaseID: Maple_MemoryManagement2.0_StartupInvoke_01
 *- @TestCaseName: StartupInvoke_01
 *- @TestCaseType: Function Testing for async TriggerGC at startup
 *- @RequirementName: GC队列实现
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:当前初始阈值是100MB，会隐式触发GC。
 *  -#step1:创建一个弱引用，检查它可以通过get()方法获得关联的引用对象。
 *  -#step2:通过1个for循环，分配内存到100M。
 *  -#step3:验证GC被隐式触发，步骤1中创建的弱引用被回收掉了。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: StartupInvoke_01.java
 *- @ExecuteClass: StartupInvoke_01
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class StartupInvoke_01 {
    static WeakReference observer;
    static int check = 2;
    private final static int INIT_MEMORY = 100; // MB
    private static ArrayList<byte[]> store = new ArrayList<byte[]>();

    public static void main(String[] args) throws Exception {
        observer = new WeakReference<Object>(new Object());
        if (observer.get() != null) {
            check--;
        }
        for (int i = 0; i < INIT_MEMORY; i++) {
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
