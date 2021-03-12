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
        // Test set/get DefaultUncaughtExceptionHandler
        ExceptionHandler eh = new ExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler(eh);
        Thread.UncaughtExceptionHandler ret1 = Thread.getDefaultUncaughtExceptionHandler();
        //Test set/get DefaultUncaughtExceptionHandler(null)
        Thread.setDefaultUncaughtExceptionHandler(null);
        Thread.UncaughtExceptionHandler ret2 = Thread.getDefaultUncaughtExceptionHandler();
        if (ret1 == eh && ret2 == null)
            System.out.println(0);
    }
    static class ExceptionHandler implements Thread.UncaughtExceptionHandler {
        public boolean wasCalled = false;
        public void uncaughtException(Thread t, Throwable e) {
            wasCalled = true;
        }
    }
}