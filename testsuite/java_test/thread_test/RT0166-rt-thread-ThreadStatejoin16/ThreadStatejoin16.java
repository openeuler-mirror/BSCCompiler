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


public class ThreadStatejoin16 extends Thread {
    static int i = 0;
    static int eCount = 0;
    public static void main(String[] args) {
        ThreadStatejoin16 tj = new ThreadStatejoin16();
        tj.start();
        try {
            tj.join(-2);
        } catch (InterruptedException e) {
            System.out.println(e);
        } catch (IllegalArgumentException ee) {
            eCount++;
        }
        try {
            tj.join(0);
        } catch (InterruptedException e) {
            System.out.println(e);
        } catch (IllegalArgumentException ee) {
            eCount = eCount + 2;
        }
        if (i == 1 && eCount == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public void run() {
        i++;
        super.run();
    }
}