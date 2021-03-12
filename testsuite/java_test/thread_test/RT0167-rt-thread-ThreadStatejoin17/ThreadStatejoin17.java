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


public class ThreadStatejoin17 extends Thread {
    static int i = 0;
    static int eCount = 0;
    public static void main(String[] args) {
        ThreadStatejoin17 tj = new ThreadStatejoin17();
        tj.start();
        long[] invalidMillis = new long[]{-2l, 2l};
        int[] invalidNanos = new int[]{2, -2};
        for (int j = 0; j < invalidMillis.length; j++) {
            try {
                tj.join(invalidMillis[j], invalidNanos[j]);
            } catch (InterruptedException e) {
                System.out.println(e);
            } catch (IllegalArgumentException ee) {
                eCount++;
            }
        }
        try {
            tj.join(0, 0);
        } catch (InterruptedException e) {
            System.out.println(e);
        } catch (IllegalArgumentException ee) {
            eCount = eCount + 10;
        }
        if (i == 1 && eCount == invalidMillis.length) {
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