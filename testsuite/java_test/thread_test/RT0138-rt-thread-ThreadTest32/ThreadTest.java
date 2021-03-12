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


public class ThreadTest {
    /**
     * Thread(ThreadGroup, Runnable, String, long) stack size is Long.MAX_VALUE.
    */

    public static void main(String[] args) throws Exception {
        // Does not work in maple, no exception occurs, run() is not executed.
        boolean expired = false;
        ThreadGroup tg = new ThreadGroup("newGroup");
        String name = "t1";
        Square s = new Square();
        System.out.println(Long.MAX_VALUE);
        try {
            Thread t = new Thread(tg, s, name, Long.MAX_VALUE);
            System.out.println("Creation_OK");
            t.start();
        } catch (OutOfMemoryError er) {
            System.out.println("OutOfMemoryError");
        }
        System.out.println("PASS");
    }
    static class Square implements Runnable {
        public void run() {
            System.out.println("Enter_OK");
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                System.out.println("unexpected InterruptedException while sleeping");
            }
        }
    }
}