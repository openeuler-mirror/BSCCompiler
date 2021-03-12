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


public class HoldsLock {
    static int eCnt = 0;
    private static Object target = null;
    private static void checkLock(boolean value) {
        if (Thread.holdsLock(target) != value)
            throw new RuntimeException("Should be " + value);
    }
    public static void main(String[] args) throws Exception {
        try {
            checkLock(false);
            throw new RuntimeException("NullPointerException not thrown");
        } catch (NullPointerException e) {
            eCnt++;
        }
        target = new Object();
        checkLock(false);
        synchronized (target) {
            checkLock(true);
        }
        checkLock(false);
        synchronized (target) {
            checkLock(true);
            new LockThread().start();
            checkLock(true);
            Thread.sleep(100);
            checkLock(true);
        }
        checkLock(false);
        if (eCnt == 1) {
            System.out.println("0");
        }
    }
    static class LockThread extends Thread {
        public void run() {
            checkLock(false);
            synchronized (target) {
                checkLock(true);
            }
            checkLock(false);
        }
    }
}