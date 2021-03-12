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
import java.lang.ref.SoftReference;
import com.huawei.ark.annotation.Weak;
class RCRunner04 implements Runnable {
    public void run() {
        RCHeaderTest04.iArray = null;
    }
}
public class RCHeaderTest04 {
    static int[] iArray;
    @Weak
    static int[] wr_annotation = null;
    static Cleaner c = null;
    static SoftReference<int[]> sr = null;
    public RCHeaderTest04() {
        iArray = new int[4];
        int[] temp = {1, 2, 3, 4};
        wr_annotation = temp;
        sr = new SoftReference<>(temp);
    }
    //方法退出后，实例变量sr声明周期结束，触发对SoftReference的处理，它指向的数组占用空间被释放。
    static void foo() {
        RCHeaderTest04 rcht = new RCHeaderTest04();
        c = Cleaner.create(rcht.sr, new RCRunner04());
        if (c == null && wr_annotation.length != 4) {
            System.out.println("Cleaner create failly");
        }
    }
    public static void main(String[] args) {
        foo();
        Runtime.getRuntime().runFinalization();
        if (c != null && wr_annotation != null && sr.get() != null) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
}
