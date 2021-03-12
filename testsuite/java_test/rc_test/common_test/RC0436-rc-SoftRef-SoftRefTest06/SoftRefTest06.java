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
import java.lang.ref.SoftReference;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class SoftRefTest06 {
    private static Reference rp;
    private static ReferenceQueue rq = new ReferenceQueue();
    private static ArrayList<byte[]> store;
    private static AtomicInteger count = new AtomicInteger(1);
    // 生命周期结束的软指针指向的对象，应该被释放，放到它的ReferenceQueue里
    public static void objectDeadNormally() {
        Reference sr = new SoftReference<SoftRefTest06>(new SoftRefTest06(), rq);
        if (sr.get() == null) {
            System.out.println("ErrorResult");
        }
        // sr will die now.
    }
    private static int oomTest() {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1024 * 1024; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
    private static void setSoftRef() {
        // oom出发，回收软引用对象使用的内存
        rp = new SoftReference<SoftRefTest06>(new SoftRefTest06(), rq);
    }
    public static void main(String[] args) {
        objectDeadNormally();
        Runtime.getRuntime().runFinalization();
        setSoftRef();
        try {
            int oom = oomTest();
        } catch (OutOfMemoryError ofm) {
            // do nothing
        }
        if (rp.get() != null) {
            System.out.println("Error Result when second check ");
        }
        Runtime.getRuntime().runFinalization();
    }
    @Override
    public void finalize() {
        System.out.println("ExpectResult" + count.getAndIncrement());
    }
}
