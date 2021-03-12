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
import java.lang.annotation.AnnotationFormatError;
import java.lang.reflect.Method;
public class MethodGetDefaultValueExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = methodGetDefaultValueExceptionTest();
        } catch (NoSuchMethodException | SecurityException e) {
            processResult -= 20;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * main test fun
     * @return return STATUS
     * @throws NoSuchMethodException
     * @throws SecurityException
    */

    public static int methodGetDefaultValueExceptionTest() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // TypeNotPresentException - if the annotation is of type Class and no definition can be found for the default
        // class value.
        // Test public Object getDefaultValue().
        Method method = RequestForEnhancementDefault.class.getMethod("cTest");
        Object value = new Object();
        try {
            value = method.getDefaultValue();
            processResult -= 10;
        } catch (AnnotationFormatError e1) {
            processResult--;
        } catch (TypeNotPresentException e1) {
            processResult--;
        }
        return result1;
    }
}
@interface RequestForEnhancementDefault {
    int id();
    String synopsis();
    String engineer() default "[assigned]";
    String date() default "[unassigned]";
    Class<Tt01> cTest() default Tt01.class;
}
/*
 * interface RequestForEnhancementDefault {
 *     int id();
 *     String synopsis();
 * //    String engineer() default "[assigned]";
 * //    String date() default "[unassigned]";
 * //    default Class<Tt01> cTest(){return   Tt01.class;}
 *     default Tt01 cTest(){return   new Tt01();}
 * }
*/

