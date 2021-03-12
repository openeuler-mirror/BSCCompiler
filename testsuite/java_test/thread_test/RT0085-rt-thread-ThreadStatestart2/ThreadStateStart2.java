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


public class ThreadStateStart2 extends Thread {
    public static void main(String[] args) {
        ThreadStateStart2 threadStateStart2 = new ThreadStateStart2();
        threadStateStart2.start();
        try {
            threadStateStart2.start();
        } catch (IllegalThreadStateException e1) {
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