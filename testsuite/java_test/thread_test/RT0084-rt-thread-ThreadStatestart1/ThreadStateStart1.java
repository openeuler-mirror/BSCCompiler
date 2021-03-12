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


public class ThreadStateStart1 extends Thread {
    static int i = 0;
    public static void main(String[] args) {
        ThreadStateStart1 threadStateStart1 = new ThreadStateStart1();
        threadStateStart1.start();
        i++;
        if (threadStateStart1.getState().toString().equals("RUNNABLE") && i == 1) {
            System.out.println(0);
        }
    }
    public void run() {
        try {
            sleep(50);
        } catch (InterruptedException e) {
        }
    }
}