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
        /**
         * Test set/get DefaultUncaughtExceptionHandler()
        */

        ExceptionHandler exceptionHandler = new ExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(exceptionHandler);
        System.out.println("The default handler has been set, should print true -- " +
                exceptionHandler.equals(Thread.getDefaultUncaughtExceptionHandler()));
        /**
         * Test set/get DefaultUncaughtExceptionHandler(null)
        */

        Thread.setDefaultUncaughtExceptionHandler(null);
        System.out.println("Default handler should be null, and the test is -- " +
                Thread.getDefaultUncaughtExceptionHandler());
        System.out.println("PASS");
    }
    static class ExceptionHandler implements Thread.UncaughtExceptionHandler {
        public boolean wasCalled = false;
        public void uncaughtException(Thread t, Throwable e) {
            wasCalled = true;
        }
    }
}