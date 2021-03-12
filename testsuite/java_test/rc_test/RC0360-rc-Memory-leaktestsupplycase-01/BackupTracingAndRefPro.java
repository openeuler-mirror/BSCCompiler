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
public class BackupTracingAndRefPro {
    static int NUM = 2;
    public static void main(String[] args) {
        runRefPro();
        Runtime.getRuntime().gc();
        runRefPro();
        runBackupTracing();
        Runtime.getRuntime().gc();
        runBackupTracing();
        System.out.println("ExpectResult");
    }
    static void runRefPro() {
        ThreadRefPro threadRefPro = new ThreadRefPro();
        for (int i = 0; i < NUM; i++) {
            threadRefPro.run();
        }
    }
    static void runBackupTracing() {
        ThreadBackupTracing threadBackupTracing = new ThreadBackupTracing();
        for (int i = 0; i < NUM; i++) {
            threadBackupTracing.run();
        }
    }
}
class ThreadBackupTracing implements Runnable {
    int result = 0;
    void versify() {
        ReferenceOperation ref = new ReferenceOperation();
        ref.referenceOperation = ref;
        ref.add();
        result = ref.sum;
    }
    @Override
    public void run() {
        versify();
        if (result != 20) {
            System.out.println("error result ==" + result);
        }
    }
}
class ReferenceOperation {
    ReferenceOperation referenceOperation;
    int num;
    int sum;
    ReferenceOperation() {
        referenceOperation = null;
        num = 10;
        sum = 0;
    }
    void add() {
        sum = num + referenceOperation.num;
    }
}
class ThreadRefPro implements Runnable {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer obj = new StringBuffer("weak");
    static void setWeakRef() {
        obj = new StringBuffer("weak");
        rp = new WeakReference<Object>(obj, rq);
        if (rp.get() == null) {
            System.out.println("error");
        }
        obj = null;
    }
    @Override
    public void run() {
        Reference reference;
        setWeakRef();
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (rp.get() != null) {
            System.out.println("error in rp.get()");
        }
        while ((reference = rq.poll()) != null) {
            if (!reference.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                System.out.println("error in rq");
            }
        }
    }
}