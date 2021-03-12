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


public class ThreadholdsLock4 extends Thread {
    static int eCount = 0;
    static int sCount = 0;
    public static void main(String[] args) {
        ThreadholdsLock4 tl = new ThreadholdsLock4();
        try {
            holdsLock(null);
        } catch (NullPointerException e) {
            eCount++;
        }
        if (!holdsLock("")) {
            if (!holdsLock("")) {
                if (!holdsLock(new Object())) {
                    sCount++;
                }
            }
        }
        if (sCount == 1 && eCount == 1) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
}