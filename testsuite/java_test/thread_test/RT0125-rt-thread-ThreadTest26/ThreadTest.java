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
    public static void main(String[] args) throws Exception {
        //Test for setUncaughtExceptionHandler()
        ThreadGroup threadGroup = new ThreadGroup("test thread group");
        Thread thread = new Thread(threadGroup, new ThreadRun("helloworld!"));
        ExceptionHandler exceptionHandler = new ExceptionHandler();
        thread.setUncaughtExceptionHandler(exceptionHandler);
        System.out.println("should be same -- " + exceptionHandler.equals(thread.getUncaughtExceptionHandler()));
        //Test for setUncaughtExceptionHandler(null)
        ThreadGroup threadGroup2 = new ThreadGroup("test thread group2");
        Thread thread2 = new Thread(threadGroup2, new ThreadRun("helloworld!"));
        thread2.setUncaughtExceptionHandler(null);
        System.out.println("Thread's thread group is expected to be a handler -- "
                + threadGroup2.equals(thread2.getUncaughtExceptionHandler()));
        System.out.println("PASS");
    }
    private static class ThreadRun implements Runnable {
        private final String helloworld;
        public ThreadRun(String str) {
            helloworld = str;
        }
        public void run() {
            System.out.println(helloworld);
        }
    }
    static class ExceptionHandler implements Thread.UncaughtExceptionHandler {
        public boolean wasCalled = false;
        public void uncaughtException(Thread t, Throwable e) {
            wasCalled = true;
        }
    }
}