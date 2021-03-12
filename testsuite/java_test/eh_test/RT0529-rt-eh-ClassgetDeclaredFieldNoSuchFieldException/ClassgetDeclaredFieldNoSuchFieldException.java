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
import java.lang.reflect.Field;
public class ClassgetDeclaredFieldNoSuchFieldException {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = classGetDeclaredFieldNoSuchFieldException();
        } catch (Exception e) {
            processResult -= 10;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * Exception in Class constructor:public Field getDeclaredField(String name)
     * @return status code
    */

    public static int classGetDeclaredFieldNoSuchFieldException() {
        int result1 = 4; /*STATUS_FAILED*/
        // NoSuchFieldException - if a field with the specified name is not found.
        //
        // public Field getDeclaredField(String name)
        Class class1 = Class.class;
        try {
            Field file1 = class1.getDeclaredField("abc123");
            processResult -= 10;
        } catch (NoSuchFieldException e1) {
            processResult--;
        }
        return result1;
    }
}
