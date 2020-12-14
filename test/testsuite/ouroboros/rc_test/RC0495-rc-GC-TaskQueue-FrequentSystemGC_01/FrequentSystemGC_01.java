/*
 *- @TestCaseID: Maple_MemoryManagement2.0_FrequentSystemGC_01
 *- @TestCaseName: FrequentSystemGC_01
 *- @TestCaseType: Function Testing for frequently system.gc() by user
 *- @RequirementName: GC队列
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:多线程同时显式触发system.gc。
 *  -#step1:创建100个线程
 *  -#step2:每个线程先sleep不同的时间，依次为1ms,2ms,3ms...
 *  -#step3:sleep完后分配一个weak, 接着显式触发gc()
 *  -#step4:每个线程在显式调用Runtime.getRuntime().gc()后，其weak都已经被释放
 *- @Expect:expected.txt
 *- @Priority: High
 *- @Source: FrequentSystemGC_01.java
 *- @ExecuteClass: FrequentSystemGC_01
 *- @ExecuteArgs:
 *- @Remark:
 */

import java.lang.ref.WeakReference;
import java.util.ArrayList;

class MyThread extends Thread {
    public void run(){
        try {
            Thread.sleep(index * FrequentSystemGC_01.THREAD_SLEEP_UNIT);
        } catch (Exception e) {
            // System.out.println("Exception from thread sleep: " + index);
            // do nothing, just continue test...
        }

        WeakReference weak = new WeakReference<Object>(new Object());
        if (weak.get() == null) {
            return;
        }
        Runtime.getRuntime().gc();
        if (weak.get() != null) {
            System.out.println("MyThread" + index + " check fail");
            FrequentSystemGC_01.totalResult = false;
        }
   }

   public MyThread(int i) {
       index = i;
   }

   private int index;
}

public class FrequentSystemGC_01 {
    public static final int THREAD_COUNT = 50;
    public static boolean totalResult = true;
    public static final long THREAD_SLEEP_UNIT = 1;
    public static ArrayList<Thread> threads = new ArrayList<>();
    public static void main(String[] args) {
        for (int i = 0; i < THREAD_COUNT; i++) {
            Thread t = new MyThread(i);
            threads.add(t);
            t.start();
        }

        for (int i = 0; i < THREAD_COUNT; i++) {
            try {
                threads.get(i).join();
            } catch (Exception e) {
                System.out.println("Exception from join thread: " + i + " need re-test!");
                return;
            }
        }
        if (FrequentSystemGC_01.totalResult == true) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, weak is not freed after system.gc() in some thread!");
        }
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
