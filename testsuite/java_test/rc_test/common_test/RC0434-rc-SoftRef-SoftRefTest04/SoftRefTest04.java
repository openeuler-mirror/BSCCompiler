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


import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
import java.util.ArrayList;
public class SoftRefTest04 {
    private static Reference rp;
    private static ReferenceQueue rq = new ReferenceQueue();
    private static int a = 100;
    private static ArrayList<byte[]> store;
    static void setSoftReference() {
        rp = new SoftReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            System.out.println("Error Result when first check ");
            a++;
        }
    }
    private static int oomTest() {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1024 * 1024; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
    public static void main(String[] args) throws Exception {
        setSoftReference();
        try {
            int Result = oomTest();
        } catch (OutOfMemoryError o) {
            // do nothing
        }
        if (rp.get() != null) {
            System.out.println("Error Result when second check ");
            a++;
        }
        Thread.sleep(500);
        Reference r = rq.poll();
        if (r == null) {
            System.out.println("Error Result when checking ReferenceQueue");
            a++;
        }
        if (a == 100) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult finally");
        }
    }
}