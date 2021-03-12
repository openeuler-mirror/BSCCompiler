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


public class ThreadGroupEHdestroy {
    static int cnt = 0;
    public static void main(String[] args) {
        ThreadGroup tg = new ThreadGroup("hi");
        Thread t = new Thread(tg, "hello");
        t.start();
        // Destroy a thread group which is not empty, throws IllegalThreadStateException
        try {
            tg.destroy();
        } catch (IllegalThreadStateException e) {
            cnt++;
        }
        try {
            t.join();
        } catch (InterruptedException ee) {
            System.out.println("Join is interrupted");
        }
        tg.destroy();
        // Destroy a thread group which is already destroyed, throws IllegalThreadStateException
        try {
            tg.destroy();
        } catch (IllegalThreadStateException e) {
            cnt += 2;
        }
        if (cnt == 3) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}