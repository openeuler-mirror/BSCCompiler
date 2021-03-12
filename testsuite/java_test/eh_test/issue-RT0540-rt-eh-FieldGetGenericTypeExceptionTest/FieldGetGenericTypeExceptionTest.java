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
import java.lang.reflect.Type;
import java.util.List;
public class FieldGetGenericTypeExceptionTest {
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
            result = fieldGetGenericTypeExceptionTest();
        } catch (Exception e) {
            e.printStackTrace();
            processResult -= 20;
        }
        //  System.out.println("============================"+result);
        // System.out.println("============================"+processResult);
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * TypeNotPresentException in reflect Field: public Type getGenericType().
     * @return status code
     * @throws NoSuchFieldException
     * @throws SecurityException
    */

    public static int fieldGetGenericTypeExceptionTest() throws NoSuchFieldException, SecurityException {
        int result1 = 4; /* STATUS_FAILED*/

        // TypeNotPresentException - if the generic type signature of the underlying field refers to a non-existent type
        // declaration.
        // Test public Type getGenericType().
        Field field = Wrapper.class.getDeclaredField("people");
        Type value = new Type() {};
        try {
            value = field.getGenericType();
            processResult -= 10;
        } catch (TypeNotPresentException e1) {
            processResult--;
        }
        return result1;
    }
}
class Wrapper {
    List<Person> people;
}
