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


import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
public class FinalizerReferenceTest03 {
    static int check = 1;
    public static void main(String[] args) {
        ReferenceQueue referenceQueue = new ReferenceQueue();
        PhantomReference<ReferenceTest03> pr = new PhantomReference<>(new ReferenceTest03(10), referenceQueue);
        if (pr.get() == null) {
            check--;
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}
class ReferenceTest03 {
    String[] stringArray;
    public ReferenceTest03(int length) {
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
