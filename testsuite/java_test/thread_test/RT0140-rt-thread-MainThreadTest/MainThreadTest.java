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


public class MainThreadTest {
    public static void main(String[] args) {
        ThreadGroup tg = Thread.currentThread().getThreadGroup();
        int n = tg.activeCount();
        Thread[] ts = new Thread[n];
        int m = tg.enumerate(ts);
        for (int i = 0; i < ts.length; i++) {
            if (Thread.currentThread() == ts[i]) {
                System.out.println(0);
                return;
            }
        }
        throw new RuntimeException("Current thread is not in its own thread group!");
    }
}