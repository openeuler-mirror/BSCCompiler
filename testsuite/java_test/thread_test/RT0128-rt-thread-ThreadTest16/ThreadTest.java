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
        while (!isInterrupted()) {
            yield();
        }
        System.out.println(isInterrupted());
        System.out.println(interrupted());
        System.out.println(isInterrupted());
    }
}
public class ThreadTest {
    public static void main(String[] args) {
        Thread thread = new MyThread();
        thread.start();
        try {
            Thread.sleep(1000);
            thread.interrupt();
        } catch (Exception e) {
            System.out.println("catch");
        }
    }
}