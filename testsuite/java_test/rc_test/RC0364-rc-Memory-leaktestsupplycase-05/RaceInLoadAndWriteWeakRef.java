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
public class RaceInLoadAndWriteWeakRef {
    static int NUM = 100;
    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
        System.out.println("ExpectResult");
    }
    static void test(){
        SetWeakRef setWeakRef = new SetWeakRef();
        setWeakRef.setWeakRef();
        ThreadLoadWeakRef threadLoadWeakRef = new ThreadLoadWeakRef(setWeakRef);
        ThreadWriteWeakRef threadWriteWeakRef = new ThreadWriteWeakRef(setWeakRef);
        for (int i = 0; i < NUM; i++) {
            threadLoadWeakRef.run();
            Runtime.getRuntime().gc();
            threadLoadWeakRef.run();
            Runtime.getRuntime().gc();
            threadLoadWeakRef.run();
            threadWriteWeakRef.run();
            Runtime.getRuntime().gc();
            threadWriteWeakRef.run();
        }
    }
}
class SetWeakRef {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer stringBuffer = new StringBuffer("Weak");
    static void setWeakRef() {
        stringBuffer = new StringBuffer("Weak");
        rp = new WeakReference(stringBuffer, rq);
        if (rp.get() == null) {
            System.out.println("error");
        }
        stringBuffer = null;
    }
}
class ThreadLoadWeakRef implements Runnable {
    SetWeakRef setWeakRef;
    public ThreadLoadWeakRef(SetWeakRef setWeakRef) {
        this.setWeakRef = setWeakRef;
    }
    static Reference rp_load;
    @Override
    public void run() {
        rp_load = setWeakRef.rp;
    }
}
class ThreadWriteWeakRef implements Runnable {
    SetWeakRef setWeakRef;
    public ThreadWriteWeakRef(SetWeakRef setWeakRef) {
        this.setWeakRef = setWeakRef;
    }
    static WeakReference rp_write;
    @Override
    public void run() {
        setWeakRef.rp = rp_write;
    }
}