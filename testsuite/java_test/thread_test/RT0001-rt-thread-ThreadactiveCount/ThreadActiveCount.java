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


public class ThreadActiveCount extends Thread {
    static int num;
    static Object ob = "aa";
    public ThreadActiveCount(ThreadGroup group, String name) {
        super(group, name);
    }
    public static void main(String[] args) {
        ThreadGroup threadGroup = new ThreadGroup("god");
        for (int i = 0; i < 10; i++) {
            (new ThreadActiveCount(threadGroup, "banana" + i)).start();
        }
        try {
            sleep(500);
        } catch (InterruptedException e1) {
            e1.printStackTrace();
        }
        if (num == 10) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
    public void run() {
        synchronized (ob) {
            num = activeCount();
            try {
                ob.wait(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}