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
import java.util.Random;
public class MonitorTestCase6 {
    private static boolean quit = false;
    private static final int INIT_DEFAULT_THREAD_NUM = 100;
    private static int DEFAULT_THREAD_NUM = INIT_DEFAULT_THREAD_NUM;
    private static final String MODULE_NAME_MONITOR = "M";
    private static int monitorLengthForCase6 = DEFAULT_THREAD_NUM / 5;
    private static boolean mRest = false;
    private static int mRestTime = 5000;
    public static void main(String[] args) {
        testCase6();
        Runtime.getRuntime().gc();
        testCase6();
        System.out.println("0");
    }
    public static void testCase6() {
        quit = false;
        ArrayList<monitorTestUnit> mList = createTestObject();
        ArrayList<Thread> list = new ArrayList<>();
        Thread t;
        for (int i = 0; i < (DEFAULT_THREAD_NUM / 10); i++) {
            t = new StopAbleThread(new SyncThread(mList), MODULE_NAME_MONITOR + "_testCase6_A" + i);
            list.add(t);
        }
    }
    public static class SyncThread implements Runnable {
        private Random intRandom;
        private ArrayList<monitorTestUnit> mList;
        public SyncThread(ArrayList<monitorTestUnit> list) {
            this.intRandom = new Random();
            this.mList = list;
        }
        @Override
        public void run() {
            monitorTestUnit monitor;
            int start;
            int sleepTime;
            for (int j = 0; j < 2000; j++) {
                start = this.intRandom.nextInt(monitorLengthForCase6);
                for (int k = 0; k < monitorLengthForCase6; k++) {
                    monitor = mList.get((k + start) % monitorLengthForCase6);
                    synchronized (monitor) {
                        tryRest();
                        monitor.testCount++;
                        sleepTime = this.intRandom.nextInt(4) + 1;
                        trySleep(sleepTime * 3);
                    }
                }
                if (quit) {
                    break;
                }
            }
        }
    }
    public static void trySleep(long time) {
        try {
            Thread.sleep(time);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    private static boolean tryRest() {
        if (mRest) {
            trySleep(mRestTime);
            return true;
        }
        return false;
    }
    public static ArrayList<monitorTestUnit> createTestObject() {
        ArrayList<monitorTestUnit> mList = new ArrayList<>();
        for (int i = 0; i < monitorLengthForCase6; i++) {
            mList.add(new monitorTestUnit());
        }
        return mList;
    }
    static class StopAbleThread extends Thread {
        public StopAbleThread(Runnable r, String name) {
            super(r, name);
        }
    }
    static class monitorTestUnit {
        int testCount;
        boolean isWait;
        monitorTestUnit() {
            testCount = 0;
            isWait = false;
        }
    }
}