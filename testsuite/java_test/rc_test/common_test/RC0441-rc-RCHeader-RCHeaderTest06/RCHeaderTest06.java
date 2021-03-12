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
import com.huawei.ark.annotation.Weak;
class RCRunner06 implements Runnable {
    public void run() {
        RCHeaderTest06.iArray = null;
    }
}
class Resource06 {
    @Override
    public void finalize() {
        System.out.println("ExpectResult");
    }
}
public class RCHeaderTest06 {
    static int[] iArray;
    static Cleaner cleaner = null;
    @Weak
    Resource06 c = null;
    public RCHeaderTest06() {
        iArray = new int[4];
        c = new Resource06();
    }
    static void foo() {
        RCHeaderTest06 rcht = new RCHeaderTest06();
        RCHeaderTest06.cleaner = Cleaner.create(rcht.c, new RCRunner06());
        if (cleaner == null) {
            System.out.println("Weak Annotation Object shouldn't be null");
        }
    }
    public static void main(String[] args) {
        foo();
        Runtime.getRuntime().runFinalization();
        System.out.println("ExpectResultEnd");
    }
}
