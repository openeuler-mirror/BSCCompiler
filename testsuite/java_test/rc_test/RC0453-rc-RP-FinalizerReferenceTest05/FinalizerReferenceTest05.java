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
public class FinalizerReferenceTest05 {
    static SoftReference<ReferenceTest05> sr05;
    static int check = 3;
    public static void main(String[] args) {
        new ReferenceTest05(10);
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        ReferenceTest05 rt05 = sr05.get();
        if (rt05.stringArray.length == 10 && rt05.stringArray[9].equals("test9")) {
            check--;
        }
        rt05 = null;
        try {
            int oom = oomTest(); // 触发oom
        } catch (OutOfMemoryError o) {
            check--;
        }
        Runtime.getRuntime().runFinalization();
        if (sr05.get() == null) {
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
class ReferenceTest05 {
    String[] stringArray;
    public ReferenceTest05(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
    @Override
    public void finalize() {
        System.out.println("ExpectResult");
        FinalizerReferenceTest05.sr05 = new SoftReference(this);
    }
}
