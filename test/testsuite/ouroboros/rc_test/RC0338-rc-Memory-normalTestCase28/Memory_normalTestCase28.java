/*
 *- @TestCaseID:rc/stress/Memory_normalTestCase28.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:
 *- @Priority: High
 *- @Source: Memory_normalTestCase28.java
 *- @ExecuteClass: Memory_normalTestCase28
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:从压力测试case中拆解 liuweiqing l00481345
 */

import java.lang.ref.Reference;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.Set;

public class Memory_normalTestCase28 {
    private static final int DEFAULT_THREAD_NUM = 80;
    public static final int LOCAL_MIN_IDX = 0;
    private static final int DEFAULT_STRESS_THREAD_NUM = 60;
    private static final String MODULE_NAME_MEMT = "MemAllocTest";
    static List<List<AllocUnit>> mAllStressThreadReference = new ArrayList<List<AllocUnit>>();
    private static ArrayList<Thread> mThreadList = new ArrayList<Thread>();

    private static void resetTestEnvirment(List<List<AllocUnit>> referenceList) {

        int size = 0;
        synchronized (referenceList) {
            size = referenceList.size();
        }

        for (; size < 2 * DEFAULT_THREAD_NUM; size++) {
            synchronized (referenceList) {
                referenceList.add(null);
            }
        }
        int i = 0;
        synchronized (referenceList) {
            for (List<AllocUnit> all : referenceList) {
                if (all != null) {
                    all.clear();
                }
                referenceList.set(i++, null);
            }
        }
    }

    public static void main(String[] args) {
        testCase28();
        Runtime.getRuntime().gc();
        testCase28();
        Runtime.getRuntime().gc();
        testCase28();
        Runtime.getRuntime().gc();
        testCase28();
        Runtime.getRuntime().gc();
        testCase28();
        System.out.println("ExpectResult");
    }

    public static void testCase28() {

        ArrayList<Thread> list = new ArrayList<Thread>();
        resetTestEnvirment(mAllStressThreadReference);
        Thread t = null;
        for (int i = 0; i < DEFAULT_THREAD_NUM / 8; i++) {
            t = new Thread(new Runnable() {

                @Override
                public void run() {
                    for (int j = 0; j < 50; j++) {
                        NativeTestFunc nativeTestFunc = new NativeTestFunc();
                        nativeTestFunc.testNativeWeakDelete(nativeTestFunc);
                        System.runFinalization();
                    }
                }
            }, MODULE_NAME_MEMT + "_testCase28_weak_" + i);
            list.add(t);
        }

        for (int i = 0; i < DEFAULT_THREAD_NUM / 8; i++) {
            t = new Thread(new Runnable() {

                @Override
                public void run() {
                    for (int j = 0; j < 50; j++) {
                        NativeTestFunc nativeTestFunc = new NativeTestFunc();
                        nativeTestFunc.testNativeGlobalDelete(nativeTestFunc);
                        System.runFinalization();
                    }
                }
            }, MODULE_NAME_MEMT + "_testCase28_global_" + i);
            list.add(t);
        }

        mThreadList.addAll(list);
        startAllThread(list);
        waitAllThreadFinish(list);

    }

    public static void waitAllThreadFinish(List<Thread> list) {
        for (Thread s : list) {
            try {
                s.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
                if (s == null)
                    return;
                if (s.isAlive()) {
                    synchronized (list) {
                        list.remove(s);
                    }
                }
            }
        }
    }

    public static void startAllThread(List<Thread> list) {
        for (Thread s : list) {
            s.start();
            trySleep(new Random().nextInt(2));
        }
    }

    public static void trySleep(long time) {
        try {
            Thread.sleep(time);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }


    public static class AllocUnit {
        public byte unit[];

        public AllocUnit(int arrayLength) {
            unit = new byte[arrayLength];
        }
    }

}

class NativeTestFunc {
    static {
       // System.loadLibrary("StressTestNative");
    }

    public native void testNativeAttach();

    public native void testNativeWeakDelete(Object input);

    public native void testNativeGlobalDelete(Object input);
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
