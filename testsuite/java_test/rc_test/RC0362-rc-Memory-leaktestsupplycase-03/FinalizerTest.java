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
import java.lang.ref.WeakReference;
public class FinalizerTest extends Object {
    static int finalizeNum = 10;
    static Reference rp[] = new Reference[finalizeNum];
    static ReferenceQueue rq = new ReferenceQueue();
    static int checkNum = 0;
    static StringBuffer stringBuffer;
    static FinalizerTest finalizerTest = null;
    static void newFinalizeObject() throws InterruptedException {
        stringBuffer = new StringBuffer("weak");
        for (int i = 0; i < finalizeNum; i++) {
            rp[i] = new WeakReference<Object>(stringBuffer, rq);
            if (rp[i].get() == null) {
                checkNum--;
            }
        }
        stringBuffer = null;
    }
    protected void finalize() throws Throwable {
        super.finalize();
        checkNum++;
    }
    static void newFinalizeTest() throws InterruptedException {
        for (int i = 0; i < finalizeNum; i++) {
            FinalizerTest finalizerTest = new FinalizerTest();
            finalizerTest.newFinalizeObject();
        }
        finalizerTest = null;
    }
    static void finalizeTest() throws InterruptedException {
        newFinalizeTest();
        Thread.sleep(2000);
        System.gc();
        System.runFinalization();
        for (int i = 0; i < finalizeNum; i++) {
            if (rp[i].get() != null) {
                checkNum--;
            }
        }
    }
    public static void main(String[] args) throws InterruptedException {
        finalizeTest();
        Runtime.getRuntime().gc();
        finalizeTest();
        if (checkNum == (finalizeNum * 2)) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("checkNum = " + checkNum);
        }
    }
}