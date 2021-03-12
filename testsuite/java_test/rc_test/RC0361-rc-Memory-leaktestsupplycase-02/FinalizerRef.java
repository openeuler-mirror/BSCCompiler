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
public class FinalizerRef {
    static int NUM = 1;
    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        System.out.println("ExpectResult");
    }
    public static void test() {
        SetWeakRef setWeakRef = new SetWeakRef();
        setWeakRef.setWeakRef();
        ThreadWeakRef_01 threadWeakRef_01 = new ThreadWeakRef_01();
        ThreadWeakRef_02 threadWeakRef_02 = new ThreadWeakRef_02();
        ThreadWeakRef_03 threadWeakRef_03 = new ThreadWeakRef_03();
        ThreadWeakRef_04 threadWeakRef_04 = new ThreadWeakRef_04();
        ThreadWeakRef_05 threadWeakRef_05 = new ThreadWeakRef_05();
        for (int i = 0; i < NUM; i++) {
            threadWeakRef_01.run();
            threadWeakRef_02.run();
            threadWeakRef_03.run();
            threadWeakRef_04.run();
            threadWeakRef_05.run();
        }
    }
}
class SetWeakRef {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer obj = new StringBuffer("weak");
    static void setWeakRef() {
        obj = new StringBuffer("weak");
        rp = new WeakReference(obj, rq);
        if (rp.get() == null) {
            System.out.println("error");
        }
        obj = null;
    }
}
class ThreadWeakRef_01 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_01 threadWeakRef_01 = new ThreadWeakRef_01();
        System.runFinalization();
    }
    protected void finalize() throws Throwable {
        super.finalize();
    }
}
class ThreadWeakRef_02 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_02 threadWeakRef_02 = new ThreadWeakRef_02();
        System.runFinalization();
    }
    protected void finalize() throws Throwable {
        super.finalize();
    }
}
class ThreadWeakRef_03 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_03 threadWeakRef_03 = new ThreadWeakRef_03();
        System.runFinalization();
    }
    protected void finalize() throws Throwable {
        super.finalize();
    }
}
class ThreadWeakRef_04 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_04 threadWeakRef_04 = new ThreadWeakRef_04();
        System.runFinalization();
    }
    protected void finalize() throws Throwable {
        super.finalize();
    }
}
class ThreadWeakRef_05 implements Runnable {
    @Override
    public void run() {
        System.gc();
        ThreadWeakRef_05 threadWeakRef_05 = new ThreadWeakRef_05();
        System.runFinalization();
    }
    protected void finalize() throws Throwable {
        super.finalize();
    }
}