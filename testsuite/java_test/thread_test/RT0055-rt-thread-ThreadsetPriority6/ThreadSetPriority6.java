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


public class ThreadSetPriority6 extends Thread {
    public ThreadSetPriority6(ThreadGroup group, String name) {
        super(group, name);
    }
    public static void main(String[] args) {
        ThreadGroup cls1 = new ThreadGroup("god");
        ThreadSetPriority6 cls2 = new ThreadSetPriority6(cls1, "good");
        cls1.setMaxPriority(3);
        if (cls1.getMaxPriority() == 3 && cls2.getPriority() == 5) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}
