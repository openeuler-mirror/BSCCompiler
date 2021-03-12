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
public class FieldgetCharExceptionInInitializerError {
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
            result = fieldGetCharExceptionInInitializerError();
        } catch (Exception e) {
            processResult -= 20;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * Exception in reflect filed:public byte getByte(Object obj)
     * @return status code
     * @throws NoSuchFieldException
     * @throws SecurityException
     * @throws IllegalArgumentException
     * @throws IllegalAccessException
    */

    public static int fieldGetCharExceptionInInitializerError()
            throws NoSuchFieldException, SecurityException, IllegalArgumentException, IllegalAccessException {
        int result1 = 4; /*STATUS_FAILED*/
        // ExceptionInInitializerError -if the initialization provoked by this method fails.
        //
        // public char getChar(Object obj)
        Field field = TestChar.class.getDeclaredField("field6");
        try {
            char obj = field.getChar(new TestChar());
            processResult -= 10;
        } catch (ExceptionInInitializerError e1) {
            processResult--;
        }
        return result1;
    }
}
class TestChar {
    /**
     * a char field for test
    */

    public static char field6 = selfChar();
    /**
     * a int[] field for test
    */

    public static int[] field2 = {1, 2, 3, 4};
    /**
     * set cValue and return
    */

    public static char selfChar() {
        int self1 = field2[2];
        char cValue = 11;
        return cValue;
    }
}
