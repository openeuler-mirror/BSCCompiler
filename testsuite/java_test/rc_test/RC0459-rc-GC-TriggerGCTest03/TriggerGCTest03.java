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
public class TriggerGCTest03 {
    static int check = 3;
    public static void main(String[] args) {
        SoftReference<Reference03> softReference = new SoftReference<>(new Reference03(100000));
        Reference03 rt01 = softReference.get();
        if (rt01.stringArray.length == 100000 && rt01.stringArray[9].equals("test9")) {
            check--;
        }
        rt01 = null;
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
            Runtime.getRuntime().runFinalization();
            if (softReference.get() == null) {
                check--;
            }
            if (check == 0) {
                System.out.println("ExpectResult");
            } else {
                System.out.println("ErrorResult");
            }
        }
    }
    private static int oomTest() {
        int sum = 0;
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1034 * 1034; i <= 1034 * 1034 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
}
class Reference03 {
    String[] stringArray;
    public Reference03(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
}
