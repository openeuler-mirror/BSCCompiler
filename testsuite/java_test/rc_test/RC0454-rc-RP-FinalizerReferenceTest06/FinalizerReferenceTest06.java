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
public class FinalizerReferenceTest06 {
    static WeakReference<ReferenceTest06> sr06;
    static int check = 2;
    public static void main(String[] args) {
        new ReferenceTest06(10);
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (sr06.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}
class ReferenceTest06 {
    String[] stringArray;
    public ReferenceTest06(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
    @Override
    public void finalize() {
        System.out.println("ExpectResult");
        FinalizerReferenceTest06.sr06 = new WeakReference<>(this);
        ReferenceTest06 rt06 = FinalizerReferenceTest06.sr06.get();
        if (rt06.stringArray.length == 10 && rt06.stringArray[9].equals("test9")) {
            FinalizerReferenceTest06.check--;
        }
    }
}
