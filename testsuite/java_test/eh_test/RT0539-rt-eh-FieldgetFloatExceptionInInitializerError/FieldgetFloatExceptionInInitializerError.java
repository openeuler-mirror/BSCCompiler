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
public class FieldgetFloatExceptionInInitializerError {
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
            result = fieldGetFloatExceptionInInitializerError();
        } catch (Exception e) {
            processResult -= 20;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * Exception in reflect filed:public float getFloat(Object obj)
     * @return status code
     * @throws NoSuchFieldException
     * @throws SecurityException
     * @throws IllegalArgumentException
     * @throws IllegalAccessException
    */

    public static int fieldGetFloatExceptionInInitializerError()
            throws NoSuchFieldException, SecurityException, IllegalArgumentException, IllegalAccessException {
        int result1 = 4; /*STATUS_FAILED*/
        // ExceptionInInitializerError -if the initialization provoked by this method fails.
        //
        // public float getFloat(Object obj)
        Field field = TestFloat.class.getDeclaredField("field6");
        try {
            float obj = field.getFloat(new TestFloat());
            processResult -= 10;
        } catch (ExceptionInInitializerError e1) {
            processResult--;
        }
        return result1;
    }
}
class TestFloat {
    /**
     * a float field for test
    */

    public static float field6 = selfDouble();
    /**
     * a int[] field for test
    */

    public static int[] field2 = {1, 2, 3, 4};
    /**
     * set fValue and return
     * @return fValue
    */

    public static float selfDouble() {
        int self1 = field2[2];
        float fValue = 11;
        return fValue;
    }
}
