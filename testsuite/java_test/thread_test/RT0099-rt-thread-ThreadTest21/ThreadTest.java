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


public class ThreadTest {
    // test for interrupt(), interrupted(), isInterrupted()   
    public static void main(String[] args) throws Exception {
        Thread thread = new Thread() {
            public void run() {
                System.out.println("helloworld executes interrupt()");
                interrupt();
                System.out.println("isInterrupted() returns -- " + isInterrupted());
                System.out.println("1st interrupted() should true -- " + Thread.interrupted());
                System.out.println("isInterrupted() returns -- " + isInterrupted());
                System.out.println("2nd interrupted() should false -- " + Thread.interrupted());
                System.out.println("isInterrupted() returns -- " + isInterrupted());
            }
        };
        thread.start();
        thread.join();
        System.out.println("PASS");
    }
}