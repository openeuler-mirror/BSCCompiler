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


import java.lang.ref.SoftReference;
import java.util.ArrayList;
public class TriggerGCTest05 {
    static int check = 4;
    public static void main(String[] args) {
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
        }
        SoftReference<Reference05> softReference = new SoftReference<>(new Reference05(100));
        Reference05 rt01 = softReference.get();
        if (rt01.stringArray.length == 100 && rt01.stringArray[9].equals("test9")) {
            check--;
        }
        rt01 = null;
        try {
            Thread.sleep(4000);
        } catch (InterruptedException e) {
            // do nothing
        }
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
        }
        if (softReference.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, check should be 0, but now check = " + check);
        }
    }
    private static int oomTest() {
        int sum = 0;
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1054 * 1054; i <= 1054 * 1054 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
}
class Reference05 {
    String[] stringArray;
    public Reference05(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
}
