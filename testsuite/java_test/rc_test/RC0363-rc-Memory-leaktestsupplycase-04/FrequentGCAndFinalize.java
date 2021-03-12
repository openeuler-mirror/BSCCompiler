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
public class FrequentGCAndFinalize {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int num = 10;
    static StringBuffer stringBuffer = new StringBuffer("weak");
    static void setWeakRef() {
        stringBuffer = new StringBuffer("weak");
        rp = new WeakReference<Object>(stringBuffer, rq);
        if (rp.get() == null) {
            System.out.println("error");
        }
        stringBuffer = null;
    }
    static void executeThreadGC() {
        ThreadGC threadGC = new ThreadGC();
        for (int i = 0; i < num; i++) {
            threadGC.run();
        }
    }
    static void executeThreadFinalize() {
        ThreadFinalize threadFinalize = new ThreadFinalize();
        for (int i = 0; i < num; i++) {
            threadFinalize.run();
        }
    }
    public static void main(String[] args) {
        setWeakRef();
        executeThreadFinalize();
        executeThreadGC();
        executeThreadFinalize();
        System.out.println("ExpectResult");
    }
}
class ThreadGC implements Runnable {
    @Override
    public void run() {
        Runtime.getRuntime().gc();
    }
}
class ThreadFinalize implements Runnable {
    @Override
    public void run() {
        ThreadFinalize threadFinalize = new ThreadFinalize();
        System.runFinalization();
    }
    protected void finalize() throws Throwable {
        super.finalize();
    }
}
