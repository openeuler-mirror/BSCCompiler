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


public class ThreadDMResume extends Thread {
    static int i = 0;
    public static void main(String[] args) {
        ThreadDMResume thread_obj = new ThreadDMResume();
        thread_obj.start();
        try {
            sleep(50);
        } catch (InterruptedException e1) {
            System.err.println(e1);
        }
        try {
            thread_obj.resume();
        } catch (UnsupportedOperationException e2) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        synchronized (this) {
            i++;
        }
    }
}