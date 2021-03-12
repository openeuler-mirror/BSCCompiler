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


public class ThreadStatewait11 extends Thread {
    public static void main(String[] args) {
        ThreadStatewait11 threadWait = new ThreadStatewait11();
        threadWait.start();
        try {
            threadWait.wait(0);
        } catch (InterruptedException e) {
            System.out.println(2);
            return;
        } catch (IllegalMonitorStateException e1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}