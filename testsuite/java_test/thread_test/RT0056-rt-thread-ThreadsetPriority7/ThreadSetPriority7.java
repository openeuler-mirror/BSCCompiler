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


public class ThreadSetPriority7 extends Thread {
    public ThreadSetPriority7(ThreadGroup group, String name) {
        super(group, name);
    }
    public static void main(String[] args) {
        ThreadGroup threadGroup = new ThreadGroup("god");
        ThreadSetPriority7 threadSetPriority7 = new ThreadSetPriority7(threadGroup, "good");
        threadGroup.setMaxPriority(3);
        threadSetPriority7.setPriority(8);
        if (threadGroup.getMaxPriority() == 3 && threadSetPriority7.getPriority() == 3) {
            System.out.println(0);
        }
    }
}