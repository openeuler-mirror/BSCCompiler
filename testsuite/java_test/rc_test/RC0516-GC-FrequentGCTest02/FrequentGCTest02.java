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


import java.lang.ref.WeakReference;
public class FrequentGCTest02 {
    static int a = 100;
    public static void main(String[] args) {
        for (int i = 0; i < 100; i++) {
            WeakReference rp = new WeakReference<Object>(new Object());
            if (rp.get() == null) {
                a++;
            }
            System.gc();
            if (rp.get() == null) {
                a++;
            }
            if (a != 100) {
                System.out.println("ErrorResult");
                return;
            }
        }
        System.out.println("ExpectResult");
    }
}
