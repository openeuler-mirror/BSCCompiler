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


import java.io.PrintStream;
public class ABCOindexAssociation {
    static int RES_PROCESS = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out)); //
    }
    public static int run(String argv[], PrintStream out) {
        int result = 4 /*STATUS_FAILED*/;
        int result2 = 4;
        try {
            result = test1();
            result2 = test2();
        } catch (Exception e) {
            RES_PROCESS -= 10;
        }
        if (result == 1 && result2 == 1 && RES_PROCESS == 99) {
            result = 0;
        }
        return result;
    }
    public static int test1() {
        int res = 3 /*STATUS_FAILED*/;
        int[] a = new int[5];
        int[] c = a;
        try {
            for (int i = 0; i <= a.length; i++) {
                c[i] = i;
            }
        } catch (ArrayIndexOutOfBoundsException e) {
            res = 1;
        }
        return res;
    }
    public static int test2() {
        int res = 3 /*STATUS_FAILED*/;
        int[] a = new int[5];
        int[] c = a;
        for (int i = 0; i < a.length; i++) {
            c[i] = i;
        }
        try {
            for (int j = 1; j <= a.length; j++) {
                if (a[j] > a[j - 1]) {
                    j = a[j];
                }
            }
        } catch (ArrayIndexOutOfBoundsException e) {
            res = 1;
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
