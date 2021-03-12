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


import java.lang.ref.*;
public class SimulThreadInRef {
    public static void main(String[] args) {
        test();
        Runtime.getRuntime().gc();
        test();
    }
    static void test() {
        int soft, weak, phantom;
        soft = testExecuteSoft();
        Runtime.getRuntime().gc();
        soft = testExecuteSoft();
        weak = testExecuteWeak();
        Runtime.getRuntime().gc();
        weak = testExecuteWeak();
        phantom = testExecutePhantom();
        Runtime.getRuntime().gc();
        phantom = testExecutePhantom();
        testExecuteOther();
        Runtime.getRuntime().gc();
        if ((soft == 0) && (weak == 0) && (phantom == 0)) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
    static int testExecuteSoft() {
        MultiRef multiRef = new MultiRef();
        ThreadTestSoftRef threadTestSoftRef = new ThreadTestSoftRef(multiRef);
        int N = 1;
        for (int i = 0; i < N; i++) {
            threadTestSoftRef.run();
        }
        return threadTestSoftRef.count1;
    }
    static int testExecuteWeak() {
        MultiRef multiRef = new MultiRef();
        ThreadTestWeakRef threadTestWeakRef = new ThreadTestWeakRef(multiRef);
        int N = 1;
        for (int i = 0; i < N; i++) {
            threadTestWeakRef.run();
        }
        return threadTestWeakRef.count2;
    }
    static int testExecutePhantom() {
        MultiRef multiRef = new MultiRef();
        ThreadTestPhantomRef threadTestPhantomRef = new ThreadTestPhantomRef(multiRef);
        int N = 1;
        for (int i = 0; i < N; i++) {
            threadTestPhantomRef.run();
        }
        return threadTestPhantomRef.count3;
    }
    static void testExecuteOther() {
        MultiRef multiRef = new MultiRef();
        ThreadTestOtherRef threadTestOtherRef = new ThreadTestOtherRef(multiRef);
        int N = 1;
        for (int i = 0; i < N; i++) {
            threadTestOtherRef.run();
        }
    }
}
class MultiRef {
    static Reference sr, wr, pr, osr;
    static StringBuffer obj1 = new StringBuffer("Soft");
    static StringBuffer obj2 = new StringBuffer("weak");
    static StringBuffer obj3 = new StringBuffer("Phantom");
    ReferenceQueue srq = new ReferenceQueue();
    ReferenceQueue wrq = new ReferenceQueue();
    ReferenceQueue prq = new ReferenceQueue();
    static int srValue, wrValue, prValue;
    void setSoftRef() {
        obj1 = new StringBuffer("Soft");
        sr = new WeakReference(obj1, srq);
        if (sr.get() == null) {
            srValue++;
        }
        obj1 = null;
    }
    void setWeakRef() {
        obj2 = new StringBuffer("weak");
        wr = new WeakReference(obj2, wrq);
        if (wr.get() == null) {
            wrValue++;
        }
        obj2 = null;
    }
    void setPhantomRef() {
        obj3 = new StringBuffer("Phantom");
        pr = new PhantomReference(obj3, prq);
        if (pr.get() != null) {
            prValue++;
        }
        obj3 = null;
    }
    void setOtherSoftRef() {
        osr = new WeakReference(new Object());
    }
    int freeSoftRef() throws InterruptedException {
        Reference r1;
        setSoftRef();
        Thread.sleep(2000);
        if (sr.get() != null) {
            srValue++;
            System.out.println("error in soft");
        }
        while ((r1 = srq.poll()) != null) {
            if (!r1.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                srValue++;
            }
        }
        return srValue;
    }
    int freeWeakRef() throws InterruptedException {
        Reference r2;
        setWeakRef();
        Thread.sleep(2000);
        if (wr.get() != null) {
            wrValue++;
            System.out.println("error in weak");
        }
        while ((r2 = wrq.poll()) != null) {
            if (!r2.getClass().toString().equals("class java.lang.ref.WeakReference")) {
                wrValue++;
            }
        }
        return wrValue;
    }
    int freePhantomRef() throws InterruptedException {
        Reference r1;
        setPhantomRef();
        Thread.sleep(2000);
        while ((r1 = prq.poll()) != null) {
            if (!r1.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
                prValue++;
            }
        }
        return prValue;
    }
}
// 线程1，设置软引用
class ThreadTestSoftRef implements Runnable {
    MultiRef multiRef;
    int count1;
    public ThreadTestSoftRef(MultiRef multiRef) {
        this.multiRef = multiRef;
    }
    @Override
    public void run() {
        try {
            count1 = multiRef.freeSoftRef();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
// 线程2，设置弱引用
class ThreadTestWeakRef implements Runnable {
    MultiRef multiRef;
    int count2;
    public ThreadTestWeakRef(MultiRef multiRef) {
        this.multiRef = multiRef;
    }
    @Override
    public void run() {
        try {
            count2 = multiRef.freeWeakRef();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
// 线程3，设置虚引用
class ThreadTestPhantomRef implements Runnable {
    MultiRef multiRef;
    int count3;
    public ThreadTestPhantomRef(MultiRef multiRef) {
        this.multiRef = multiRef;
    }
    @Override
    public void run() {
        try {
            count3 = multiRef.freePhantomRef();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
// 线程4，设置其他引用
class ThreadTestOtherRef implements Runnable {
    MultiRef multiRef;
    public ThreadTestOtherRef(MultiRef multiRef) {
        this.multiRef = multiRef;
    }
    @Override
    public void run() {
        multiRef.setOtherSoftRef();
    }
}