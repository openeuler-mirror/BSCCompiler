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


import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.Arrays;
public class MethodExExecutablegetGenericExceptionTypesExceptions {
    static int res = 99;
    public static void main(String[] argv) {
        System.out.println(new MethodExExecutablegetGenericExceptionTypesExceptions().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = methodExExecutablegetGenericExceptionTypesExceptions1();
        } catch (Exception e) {
            MethodExExecutablegetGenericExceptionTypesExceptions.res = MethodExExecutablegetGenericExceptionTypesExceptions.res - 20;
        }
        if (result == 4 && MethodExExecutablegetGenericExceptionTypesExceptions.res == 89) {
            result = 0;
        }
        return result;
    }
    private int methodExExecutablegetGenericExceptionTypesExceptions1() throws NoSuchMethodException, InvocationTargetException, IllegalAccessException {
        //  Type[] getGenericExceptionTypes()
        int result1 = 4;
        Method m1 = SampleClassH111.class.getDeclaredMethod("getSampleField");
        try {
            Type[] tp = m1.getGenericExceptionTypes();
            if (Arrays.toString(tp).equals("[class java.lang.ArrayIndexOutOfBoundsException]")) {
                MethodExExecutablegetGenericExceptionTypesExceptions.res = MethodExExecutablegetGenericExceptionTypesExceptions.res - 10;
            }
        } catch (Exception e) {
            MethodExExecutablegetGenericExceptionTypesExceptions.res = MethodExExecutablegetGenericExceptionTypesExceptions.res - 15;
        }
        return result1;
    }
}
@CustomAnnotationsH111(name = "SampleClass", value = "Sample Class Annotation")
class SampleClassH111 {
    private String sampleField;
    public String getSampleField() throws ArrayIndexOutOfBoundsException {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@interface CustomAnnotationsH111 {
    String name();
    String value();
}
