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


public class Threadenumerate2 extends Thread {
    static int num = 0;
    static int eCount = 0;
    public static void main(String[] args) {
        Threadenumerate2 te = new Threadenumerate2();
        te.start();
        try {
            enumerate(null);
        } catch (NullPointerException e) {
            eCount++;
        }
        num = enumerate(new Thread[0]);
        if (num == 0 && eCount == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}