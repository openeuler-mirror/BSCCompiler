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
public class FinalizerReferenceTest02 {
    static int check = 2;
    public static void main(String[] args) {
        WeakReference<ReferenceTest02> weakReference = new WeakReference<>(new ReferenceTest02(10));
        ReferenceTest02 rt02 = weakReference.get();
        if (rt02.stringArray.length == 10 && rt02.stringArray[9].equals("test9")) {
            check--;
        }
        rt02 = null;
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (weakReference.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}
class ReferenceTest02 {
    String[] stringArray;
    public ReferenceTest02(int length) {
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
