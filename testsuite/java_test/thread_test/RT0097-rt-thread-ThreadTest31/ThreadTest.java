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


public class ThreadTest extends Thread {
    public static void main(String[] args) throws Exception {
        ThreadTest threadTest = new ThreadTest();
        threadTest.start();
        threadTest.join();
        System.out.println("PASS");
    }
    //test for call of static method interrupted(), and method interrupt()
    // in java a static method can be called by className.staticMethod and by objectName.staticMethod
    public void run() {
        System.out.println("isInterrupted() returns -- " + isInterrupted());
        interrupt();
        System.out.println("after executing interrupt()");
        System.out.println("isInterrupted() returns -- " + isInterrupted());
        System.out.println("1st should true -- " + interrupted());
        System.out.println("isInterrupted() returns -- " + isInterrupted());
        System.out.println("2nd should false -- " + interrupted());
        System.out.println("isInterrupted() returns -- " + isInterrupted());
    }
}