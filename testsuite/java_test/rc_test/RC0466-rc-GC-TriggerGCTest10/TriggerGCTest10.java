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
import java.util.ArrayList;
public class TriggerGCTest10 {
    static WeakReference observer;
    static int check = 2;
    private final static int TRI_MEMORY = 225; // MB
    private final static int FOU_MEMORY = 350; // MB
    private static ArrayList<byte[]> store = new ArrayList<byte[]>();
    public static void main(String[] args) throws Exception {
        for (int i = 0; i < TRI_MEMORY; i++) {
            byte[] temp = new byte[1024*1024];
            store.add(temp);
        }
        Thread.sleep(2000);
        observer = new WeakReference<Object>(new Object());
        if (observer.get() != null) {
            check--;
        }
        try {
            for (int i = TRI_MEMORY; i < FOU_MEMORY; i++) {
                byte[] temp = new byte[1024 * 1024];
                store.add(temp);
            }
        } catch (OutOfMemoryError e) {
            // do nothing
        }
        Thread.sleep(1000);
        if (observer.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, check should be 0, but now check = " + check);
        }
    }
}
