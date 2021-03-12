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


public class ThreadSetPriority1 extends Thread {
    public static void main(String[] args) {
        ThreadSetPriority1 thread_obj1 = new ThreadSetPriority1();
        ThreadSetPriority1 thread_obj2 = new ThreadSetPriority1();
        ThreadSetPriority1 thread_obj3 = new ThreadSetPriority1();
        ThreadSetPriority1 thread_obj4 = new ThreadSetPriority1();
        thread_obj1.setPriority(6);
        thread_obj4.setPriority(MIN_PRIORITY);
        thread_obj2.setPriority(MAX_PRIORITY);
        thread_obj3.setPriority(3);
        thread_obj1.setPriority(2);
        thread_obj2.setPriority(MIN_PRIORITY);
        thread_obj3.setPriority(NORM_PRIORITY);
        thread_obj4.setPriority(7);
        thread_obj1.start();
        thread_obj2.start();
        thread_obj3.start();
        thread_obj4.start();
        if (thread_obj1.getPriority() != 2) {
            System.out.println(2);
        }
        if (thread_obj2.getPriority() != 1) {
            System.out.println(2);
        }
        if (thread_obj3.getPriority() != 5) {
            System.out.println(2);
        }
        if (thread_obj4.getPriority() != 7) {
            System.out.println(2);
        }
        System.out.println(0);
    }
}
