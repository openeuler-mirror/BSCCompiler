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
import java.util.Arrays;
public class ABCOstringMutiLoop {
    static int RES_PROCESS = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 4 /*STATUS_FAILED*/;
        try {
            result = test1();
        } catch (Exception e) {
            RES_PROCESS -= 10;
        }
        if (result == 1 && RES_PROCESS == 99) {
            result = 0;
        }
        return result;
    }
    public static int test1() {
        int res = 2 /*STATUS_FAILED*/;
        char[] a = new char[8000];
        for (int i = 0; i < a.length; i++) {
            a[i] = 'h';
        }
        String joinLine = Arrays.toString(a);
        int x;
        try {
            x = func();
            char c = joinLine.charAt(x);
        } catch (StringIndexOutOfBoundsException e) {
            res--;
        }
        return res;
    }
    public static int func() {
        int index = 0;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                for (int k = 0; k < 5; k++) {
                    for (int ii = 0; ii < 5; ii++) {
                        for (int jj = 0; jj < 5; jj++) {
                            for (int kk = 0; kk < 5; kk++) {
                                for (int iii = 0; iii < 5; iii++) {
                                    for (int jjj = 0; jjj < 5; jjj++) {
                                        for (int kkk = 0; kkk < 5; kkk++) {
                                            for (int n = 0; n < 5; n++) {
                                                index = index + n;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return index;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
