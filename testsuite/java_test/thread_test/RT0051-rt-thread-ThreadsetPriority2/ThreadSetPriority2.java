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


class ThreadSetPriority_a extends Thread {
    public synchronized void run() {
        for (int i = 0; i < 10; i++) {
        }
    }
}
public class ThreadSetPriority2 extends ThreadSetPriority_a {
    public static void main(String[] args) {
        currentThread().setPriority(2);
        ThreadSetPriority2 thread_obj1 = new ThreadSetPriority2();
        ThreadSetPriority2 thread_obj2 = new ThreadSetPriority2();
        thread_obj1.start();
        thread_obj2.start();
        if (thread_obj1.getPriority() != 2) {
            System.out.println(2);
            return ;
        }
        if (thread_obj2.getPriority() != 2) {
            System.out.println(2);
            return ;
        }
        System.out.println(0);
        return;
    }
}
