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


class ThreadTest_24 {
    /* Test for interrupt() -- Interrupt the current thread
     * If this thread is blocked in an invocation
     * of the wait(), wait(long), or wait(long, int) methods of the Object class, or
     * of the join(), join(long), join(long, int), sleep(long), or sleep(long, int), methods of this class,
     * then its interrupt status will be cleared and it will receive an InterruptedException.
    */

    public static void main(String[] args) {
        Thread t = new Thread() {
            public void run() {
                interrupt();
                System.out.println("in the new thread, isInterrupted() returns -- " + isInterrupted());
            }
        };
        t.start();
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            System.out.println("unexpected InterruptedException while sleeping");
        }
        System.out.println("isInterrupted() finally returns -- " + t.isInterrupted());
        System.out.println("PASS");
    }
}