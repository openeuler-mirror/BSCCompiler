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
public class FinalizerReferenceTest07 {
    static ReferenceQueue referenceQueue = new ReferenceQueue();
    static PhantomReference<ReferenceTest07> sr07;
    static int check = 2;
    public static void main(String[] args) {
        new ReferenceTest07(10);
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        if (sr07.get() == null) {
            check--;
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // do nothing
        }
        if (referenceQueue.poll() != null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}
class ReferenceTest07 {
    String[] stringArray;
    public ReferenceTest07(int length) {
        stringArray = new String[length];
        for (int i = 0; i < length; i++) {
            stringArray[i] = "test" + i;
        }
    }
    @Override
    public void finalize() {
        System.out.println("ExpectResult");
        FinalizerReferenceTest07.sr07 = new PhantomReference<>(this, FinalizerReferenceTest07.referenceQueue);
    }
}
