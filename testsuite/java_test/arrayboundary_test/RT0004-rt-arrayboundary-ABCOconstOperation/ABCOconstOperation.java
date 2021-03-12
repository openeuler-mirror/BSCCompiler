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
public class ABCOconstOperation {
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
        int res = 10 /*STATUS_FAILED*/;
        int[] arr1 = new int[15];
        for (int i = 0; i < arr1.length; i++) {
            arr1[i] = i;
        }
        int index = 1;
        int y = 6;
        try {
            index = index + y + 9;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = y - 16 - index;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = y * y * index;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = index + 937;
            index = (y / index) * y;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = (y + 9) & 15;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = y | index;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = ~index;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = 15;
            index = index ^ 16;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        try {
            index = index % 16;
            int c = arr1[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
