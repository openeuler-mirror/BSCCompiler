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


public class NullStackTrace {
    static final int TIMES = 1000;
    public static void main(String[] args) {
        for (int i = 0; i < TIMES; i++) {
            Thread t = new Thread();
            t.start();
            StackTraceElement[] ste = t.getStackTrace();
            if (ste == null)
                throw new RuntimeException("Failed: Thread.getStackTrace should not return null");
        }
        System.out.println("Passed");
    }
}