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
import com.huawei.ark.annotation.Weak;
class RCRunner03 implements Runnable {
    public void run() {
        RCHeaderTest03.iArray = null;
    }
}
public class RCHeaderTest03 {
    static String[] iArray;
    private static ArrayList<byte[]> store;
    String @Weak [] wr_annotation = null;
    Cleaner c = null;
    SoftReference<String[]> sr = null;
    public RCHeaderTest03() {
        iArray = new String[4];
        String[] temp = {"1", "2", "3", "4"};
        wr_annotation = temp;
        sr = new SoftReference<>(temp);
    }
    //方法退出后，实例变量sr声明周期结束，触发对SoftReference的处理，它指向的数组占用空间被释放。
    void foo() {
        c = Cleaner.create(wr_annotation, new RCRunner03());
        if (c == null || wr_annotation.length != 4) {
            System.out.println("Cleaner create failly");
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
        RCHeaderTest03 rcht = new RCHeaderTest03();
        rcht.foo();
        try {
            int oom = oomTest();
        } catch (OutOfMemoryError ofm) {
            System.out.println("oom occured");
        }
        Runtime.getRuntime().gc();
        if (rcht.sr.get() == null ) {
            System.out.println("SR release");
        }
        try {
            Thread.sleep(5000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (rcht.wr_annotation == null) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}