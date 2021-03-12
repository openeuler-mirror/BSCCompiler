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
import java.lang.reflect.Array;
import java.util.Arrays;
public class ABCOincreaseArray {
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
        int[] a = new int[10];
        Arrays.fill(a, 20);
        int[] newArray = (int[]) func(a);
        int index = newArray[0];
        try {
            int c = a[index - 11];
        } catch (ArrayIndexOutOfBoundsException e) {
            res = 5;
        }
        try {
            int c = newArray[index];
        } catch (ArrayIndexOutOfBoundsException e) {
            res--;
        }
        return res;
    }
    public static Object func(Object array) {
        Class<?> clazz = array.getClass();
        if (clazz.isArray()) {
            Class<?> componentType = clazz.getComponentType();
            int length = Array.getLength(array);
            Object newArray = Array.newInstance(componentType, length + 10);
            System.arraycopy(array, 0, newArray, 0, length);
            return newArray;
        }
        return null;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
