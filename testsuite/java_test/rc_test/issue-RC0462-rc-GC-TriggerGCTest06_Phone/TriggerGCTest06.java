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


import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
public class TriggerGCTest06 {
    static WeakReference observer;
    static ReferenceQueue rq = new ReferenceQueue();
    static int check = 2;
    private final static int RPLENGTH = 10000;
    public static void main(String[] args) throws Exception {
        observer = new WeakReference<Object>(new Object());
        if (observer.get() != null) {
            check--;
        }
        for (int i = 0; i < RPLENGTH; i++) {
            WeakReference wr = new WeakReference(new Object(),rq);
            wr = null;
        }
        Thread.sleep(3000);
        if (observer.get() == null) { // gconly因为该触发机制已经去使能，所以不会隐式触发GC; rc使能了该机制。
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, check should be 0, but now check = " + check);
        }
    }
}
