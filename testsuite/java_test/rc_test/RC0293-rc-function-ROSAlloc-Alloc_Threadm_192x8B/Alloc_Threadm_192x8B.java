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


import java.util.ArrayList;
class Alloc_Threadm_192x8B_01 extends Thread {
    private static final int PAGE_SIZE = 4 * 1024;
    private static final int OBJ_HEADSIZE = 8;
    private static final int MAX_192_8B = 192 * 8;
    private static boolean checkout = false;
    public void run() {
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;
        int checkSize = 0;
        for (int i = 1024 - OBJ_HEADSIZE; i <= MAX_192_8B - OBJ_HEADSIZE; i = i + 100) {
            for (int j = 0; j < (PAGE_SIZE * 512 / (i + OBJ_HEADSIZE) + 10); j++) {
                temp = new byte[i];
                store.add(temp);
                checkSize++;
                store = new ArrayList<byte[]>();
            }
        }
        if (checkSize == 10117) {
            checkout = true;
        }
    }
    public boolean check() {
        return checkout;
    }
}
public class Alloc_Threadm_192x8B {
    public static void main(String[] args) {
        Runtime.getRuntime().gc();
        Alloc_Threadm_192x8B_01 test1 = new Alloc_Threadm_192x8B_01();
        test1.start();
        Alloc_Threadm_192x8B_01 test2 = new Alloc_Threadm_192x8B_01();
        test2.start();
        Alloc_Threadm_192x8B_01 test3 = new Alloc_Threadm_192x8B_01();
        test3.start();
        Alloc_Threadm_192x8B_01 test4 = new Alloc_Threadm_192x8B_01();
        test4.start();
        try {
            test1.join();
            test2.join();
            test3.join();
            test4.join();
        } catch (InterruptedException e) {
            // do nothing
        }
        if (test1.check() && test2.check() && test3.check() && test4.check()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("Error");
        }
    }
}
