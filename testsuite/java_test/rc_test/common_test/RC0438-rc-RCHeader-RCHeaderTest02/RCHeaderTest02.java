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


import sun.misc.Cleaner;
import java.util.ArrayList;
import java.lang.ref.SoftReference;
class RCRunner02 implements Runnable {
    public void run() {
        RCHeaderTest02.iArray = null;
    }
}
class Resource02 {
    @Override
    public void finalize() {
        System.out.println("ExpectResult");
    }
}
public class RCHeaderTest02 {
	private static ArrayList<byte[]> store;
    static int[] iArray;
    static Cleaner c = null;
    Resource02 res02 = null;
    public RCHeaderTest02() {
        iArray = new int[4];
        res02 = new Resource02();
    }
    //方法退出后，sr_rcht02声明周期结束，触发对SoftReference的处理，它指向的对象占用空间被释放。
    static void foo() {
        SoftReference<RCHeaderTest02> sr_rcht02 = new SoftReference<>(new RCHeaderTest02());
        c = Cleaner.create(sr_rcht02, new RCRunner02());
        if (sr_rcht02.get() == null || c == null) {
            System.out.println("SoftReference error or Cleaner create failly");
        }
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
    public static void main(String[] args) {
        foo();
        try {
            int oom = oomTest();
        } catch (OutOfMemoryError ofm) {}
        Runtime.getRuntime().runFinalization();
        System.out.println("ExpectResultEnd");    //ReleaseEnd
    }
}
