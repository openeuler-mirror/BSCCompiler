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


class ThreadExtends2 extends Thread {
    private int ticket = 10;
    public static void main(String[] args) {
        ThreadExtends2 cls1 = new ThreadExtends2();
        ThreadExtends2 cls2 = new ThreadExtends2();
        ThreadExtends2 cls3 = new ThreadExtends2();
        try {
            cls1.start();
            cls2.start();
            cls3.start();
        } catch (Exception e) {
            System.out.println(2);
        }
        System.out.println(0);
    }
    public synchronized void run() {
        for (int i = 0; i < 20; i++) {
            if (this.ticket > 0) {
                this.ticket--;
            } else {
                break;
            }
        }
    }
}