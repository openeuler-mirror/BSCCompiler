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


public class ThreadGetContextClassLoader {
    public static void main(String[] args) {
        Thread thr = new Thread();
        try {
            // If not set, the default is the ClassLoader context of the parent Thread.
            if(thr.getContextClassLoader() != Thread.currentThread().getContextClassLoader()) {
                System.out.println(2);
            }
        } finally {
            thr.start();
            try {
                thr.join();
            } catch(InterruptedException e) {
                System.out.println("Join is interrupted");
            }
        }
        System.out.println(0);
    }
}