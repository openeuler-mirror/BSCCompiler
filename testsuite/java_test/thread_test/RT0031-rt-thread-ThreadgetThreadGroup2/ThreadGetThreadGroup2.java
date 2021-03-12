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


public class ThreadGetThreadGroup2 extends Thread {
    public ThreadGetThreadGroup2(ThreadGroup group, String name) {
        super(group, name);
    }
    public static void main(String[] args) {
        String string;
        ThreadGroup threadGroup = new ThreadGroup("god");
        threadGroup.setMaxPriority(7);
        ThreadGetThreadGroup2 threadGetThreadGroup2 = new ThreadGetThreadGroup2(threadGroup, "banana");
        threadGetThreadGroup2.start();
        try {
            threadGetThreadGroup2.join();
        } catch (InterruptedException e1) {
            System.out.println("Join is interrupted");
        }
        try {
            string = threadGetThreadGroup2.getThreadGroup().toString();
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.out.println(0);
        }
    }
}