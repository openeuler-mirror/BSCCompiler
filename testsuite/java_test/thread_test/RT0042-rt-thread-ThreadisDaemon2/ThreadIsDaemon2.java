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


public class ThreadIsDaemon2 extends Thread {
    public static void main(String[] args) {
        ThreadIsDaemon2 threadIsDaemon2 = new ThreadIsDaemon2();
        threadIsDaemon2.setDaemon(true);
        threadIsDaemon2.start();
        if (currentThread().isDaemon()) {
            System.out.println(2);
            return;
        }
        System.out.println(0);
    }
}