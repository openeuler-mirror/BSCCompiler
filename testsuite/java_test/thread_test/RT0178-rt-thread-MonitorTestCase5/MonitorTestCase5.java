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


import java.util.ArrayList;
import java.util.List;
import java.util.Random;
public class MonitorTestCase5 {
    private static final int INIT_DEFAULT_THREAD_NUM = 100;
    private static int DEFAULT_THREAD_NUM_HALF = INIT_DEFAULT_THREAD_NUM / 2;
    private static final int THREAD_SLEEP_TIME_DEFAULT = 1;
    private static final int THREAD_REPEATS_INFINITE = -1;
    private static final int THREAD_REPEATS_DEFAULT = 1;
    private static final String MODULE_NAME_MONITOR = "M";
    private static boolean mRunning = true;
    private static boolean mRest = false;
    private static int mRestTime = 5000;
    static List<Thread> mAllThread = new ArrayList<>();
    public static void main(String[] args) {
        testCase5();
        Runtime.getRuntime().gc();
        testCase5();
        System.out.println("0");
    }
    public static void testCase5() {
        ArrayList<Thread> list = new ArrayList<>();
        Thread t1;
        for (int i = 0; i < DEFAULT_THREAD_NUM_HALF; i++) {
            t1 = new StopAbleThread(new CommonRun(mTestVolatileRun, THREAD_SLEEP_TIME_DEFAULT, THREAD_REPEATS_DEFAULT),
                    MODULE_NAME_MONITOR + "_testCase5_" + i);
            list.add(t1);
        }
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
                removeDeadThread(s);
            }
        }
    }
    public static void removeDeadThread(Thread t) {
        if (t == null) {
            return;
        }
        if (!t.isAlive()) {
            synchronized (mAllThread) {
                mAllThread.remove(t);
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
    private static Runnable mTestVolatileRun = new Runnable() {
        @Override
        public void run() {
            for (int i = 0; i < 100; i++) {
                tryRest();
                Object testa = GetTestVolatile();
                if (testa instanceof Object) {
                    continue;
                }
            }
        }
    };
    public static volatile Object testVolatile = null;
    private static Object GetTestVolatileImpl() {
        if (testVolatile == null) {
            testVolatile = new testVolatileClass();
        }
        return testVolatile;
    }
    static class testVolatileClass {
        // test class
    }
    private static Object GetTestVolatile() {
        return GetTestVolatileImpl();
    }
    private static boolean tryRest() {
        if (mRest) {
            trySleep(mRestTime);
            return true;
        }
        return false;
    }
    static class CommonRun implements Runnable {
        int sleepTime;
        int repeats;
        Runnable cbFun;
        public CommonRun(Runnable cb, int sleepTime, int repeatTimes) {
            this.sleepTime = sleepTime;
            repeats = repeatTimes;
            cbFun = cb;
        }
        @Override
        public void run() {
            while (THREAD_REPEATS_INFINITE == repeats || repeats > 0) {
                if (repeats > 0) {
                    repeats--;
                }
                tryRest();
                if (cbFun != null) {
                    cbFun.run();
                }
                trySleep(sleepTime);
                if (!mRunning) {
                    break;
                }
            }
        }
    }
    static class StopAbleThread extends Thread {
        public StopAbleThread(Runnable r, String name) {
            super(r, name);
        }
    }
}