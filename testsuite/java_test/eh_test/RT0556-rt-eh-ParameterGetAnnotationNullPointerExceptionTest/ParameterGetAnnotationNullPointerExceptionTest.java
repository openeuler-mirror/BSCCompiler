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
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;
public class ParameterGetAnnotationNullPointerExceptionTest {
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
            result = parameterGetAnnotationNullPointerException();
        } catch (Exception e) {
            processResult -= 20;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * Exception in reflect.Parameter: public T getAnnotation(Class<T> annotationClass).
     * @return status code
     * @throws NoSuchMethodException
     * @throws SecurityException
    */

    public static int parameterGetAnnotationNullPointerException() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        Method method = Test03a.class.getMethod("getName", new Class[] {String.class});
        Parameter[] parameters = method.getParameters();
        try {
            Object value = parameters[0].getAnnotation(null);
            processResult -= 10;
        } catch (NullPointerException e1) {
            processResult--;
        }
        return result1;
    }
}
class Test03a {
    private String name = "default";
    public String getName(String str1) {
        return str1;
    }
}
