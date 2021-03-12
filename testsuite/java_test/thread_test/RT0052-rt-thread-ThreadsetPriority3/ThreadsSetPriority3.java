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


public class ThreadsSetPriority3 extends Thread {
    public static void main(String[] args) {
        ThreadsSetPriority3 thread_obj1 = new ThreadsSetPriority3();
        ThreadsSetPriority3 thread_obj2 = new ThreadsSetPriority3();
        ThreadsSetPriority3 thread_obj3 = new ThreadsSetPriority3();
        ThreadsSetPriority3 thread_obj4 = new ThreadsSetPriority3();
        try {
            thread_obj1.setPriority(11);
            System.out.println(2);
        } catch (IllegalArgumentException e1) {
            try {
                thread_obj2.setPriority(0);
                System.out.println(2);
            } catch (IllegalArgumentException e2) {
                try {
                    thread_obj3.setPriority(MAX_PRIORITY + 1);
                    System.out.println(2);
                } catch (IllegalArgumentException e3) {
                    try {
                        thread_obj4.setPriority(MIN_PRIORITY - 1);
                        System.out.println(2);
                    } catch (IllegalArgumentException e4) {
                        System.out.println(0);
                    }
                }
            }
        }
    }
}
