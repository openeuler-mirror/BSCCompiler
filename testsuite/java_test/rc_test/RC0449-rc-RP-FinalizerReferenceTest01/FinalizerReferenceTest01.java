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
public class FinalizerReferenceTest01 {
    static int check = 3;
    public static void main(String[] args) {
        SoftReference<ReferenceTest01> softReference = new SoftReference<>(new ReferenceTest01(10));
        ReferenceTest01 rt01 = softReference.get();
        if (rt01.stringArray.length == 10 && rt01.stringArray[9].equals("test9")) {
            check--;
        }
        rt01 = null;
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
        }
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
    private static int oomTest() {
        int sum = 0;
        ArrayList<byte[]> store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1024 * 1024; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
}
class ReferenceTest01 {
    String[] stringArray;
    public ReferenceTest01(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
    @Override
    public void finalize() {
        for (int i = 0; i < this.stringArray.length; i++) {
            stringArray[i] = null;
        }
        System.out.println("ExpectResult");
    }
}
