/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/


import java.lang.ref.Reference;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;
import java.util.*;
public class Memory_normalTestCase27 {
    static final int PAGE_SIZE = 4016;
    static final int PAGE_HEADSIZE = 80;
    static final int OBJ_MAX_SIZE = 1008;
    static final int OBJ_HEADSIZE = 16;
    private static final int DEFAULT_THREAD_NUM = 80;
    static final int MAX_SLOT_NUM = 62;
    private static boolean mRest = false;
    private static int mRestTime = 5000;
    static boolean mRunning = true;
    static final int LOCAL_MAX_IDX = 15;
    public static final int LOCAL_MIN_IDX = 0;
    public static final int GLOBAL_MAX_IDX = 62;
    static final int GLOBAL_MIN_IDX = 16;
    static final int LARGE_PAGE_SIZE = 4096;
    static final int LARGEOBJ_CHOSEN_NUM = 2;
    static final int LARGEOBJ_ALLOC_SIZE = 128 * LARGE_PAGE_SIZE;
    private static final int DEFAULT_REPEAT_TIMES = 1;
    private static final int ALLOC_16K = 16 * 1024;
    private static final int ALLOC_12K = 12 * 1024;
    private static final int ALLOC_8K = 8 * 1024;
    private static final int ALLOC_4K = 4 * 1024;
    private static final int ALLOC_2K = 2 * 1024;
    private static final int DEFAULT_STRESS_THREAD_NUM = 60;
    private static final int DEFAULT_STRESS_THREAD_NUM_HALF = DEFAULT_STRESS_THREAD_NUM / 2;
    private static ArrayList<Thread> mThreadList = new ArrayList<Thread>();
    public static void testCase27() {
        RCWeakProxyTest.RCWeakProxyTestEntry();
    }
    public static void main(String[] args) {
        testCase27();
        System.out.println("ExpectResult");
    }
}
class RCWeakProxyTest {
    static class A {
        Object o;
    }
    static class ThreadRunning extends Thread {
        HashMap map;
        Hashtable table;
        ThreadRunning(HashMap m, Hashtable t) {
            super("testCase_RCWeakProxy");
            map = m;
            table = t;
        }
        int foo() {
            return map.values().size();
        }
        public void run() {
            for (int i = 0; i < 500; i++) {
                /*
                 * Collection c = map.values(); if (c == null) {
                 * System.out.println("error"); return; } int x = c.size();
               */


                int x = foo();
                if (x == 100) {
                } else {
                    A a = new A();
                    a = new A();
                }
                /*
                 * int y = table.values().size(); if (y == 100) {
                 * System.out.println("bla"); }
               */


            }
        }
    }
    public static void RCWeakProxyTestEntry() {
        HashMap map = new HashMap();
        map.put("key1", "value1");
        Hashtable table = new Hashtable();
        table.put("key1", "value1");
        int num_thread = 2;
        ThreadRunning t[] = new ThreadRunning[num_thread];
        for (int i = 0; i < num_thread; i++) {
            t[i] = new ThreadRunning(map, table);
            t[i].start();
        }
        try {
            for (int i = 0; i < num_thread; i++) {
                t[i].join();
            }
        } catch (Exception e) {
        }
    }
}
