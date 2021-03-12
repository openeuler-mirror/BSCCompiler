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


class MyThread extends Thread {
    public void run() {
        try {
            while (!isInterrupted()) {
                Thread.sleep(10000);
            }
        } catch (Exception e) {
            System.out.println("isInterrupted");
        }
    }
}
public class ThreadTest {
    public static void main(String[] args) {
        Thread t1 = new MyThread();
        t1.start();
        try {
            Thread.sleep(1000);
            t1.interrupt();
        } catch (Exception e) {
            System.out.println("catch");
        }
    }
}