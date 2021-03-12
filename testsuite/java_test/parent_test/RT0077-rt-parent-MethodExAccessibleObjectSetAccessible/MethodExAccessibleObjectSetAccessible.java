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


import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Method;
public class MethodExAccessibleObjectSetAccessible {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = methodExAccessibleObjectSetAccessible1();
        } catch (Exception e) {
            MethodExAccessibleObjectSetAccessible.res = MethodExAccessibleObjectSetAccessible.res - 10;
        }
        try {
            result = methodExAccessibleObjectSetAccessible2();
        } catch (Exception e) {
            MethodExAccessibleObjectSetAccessible.res = MethodExAccessibleObjectSetAccessible.res - 10;
        }
        if (result == 4 && MethodExAccessibleObjectSetAccessible.res == 79) {
            result = 0;
        }
        return result;
    }
    private static int methodExAccessibleObjectSetAccessible1() throws NoSuchFieldException {
        int result1 = 4; /*STATUS_FAILED*/
        // SecurityException - Exception safety
        // static void setAccessible(AccessibleObject[] array, boolean flag)
        Method[] sampleMethod = MethodClass14.class.getMethods();
        try {
            AccessibleObject.setAccessible(sampleMethod, false);
            MethodExAccessibleObjectSetAccessible.res = MethodExAccessibleObjectSetAccessible.res - 10;
        } catch (SecurityException e1) {
            MethodExAccessibleObjectSetAccessible.res = MethodExAccessibleObjectSetAccessible.res - 1;
        }
        return result1;
    }
    private static int methodExAccessibleObjectSetAccessible2() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // SecurityException - Exception safety
        //  void setAccessible(boolean flag)
        Method sampleMethod = MethodClass14.class.getMethod("sampleMethod");
        try {
            sampleMethod.setAccessible(false);
            MethodExAccessibleObjectSetAccessible.res = MethodExAccessibleObjectSetAccessible.res - 10;
        } catch (SecurityException e1) {
            MethodExAccessibleObjectSetAccessible.res = MethodExAccessibleObjectSetAccessible.res - 1;
        }
        return result1;
    }
}
@CustomAnnotationwe(name = "SampleClass", value = "Sample Class Annotation")
class MethodClass14 {
    private String sampleField;
    @CustomAnnotationwe(name = "sampleMethod", value = "Sample Method Annotation")
    public String sampleMethod() {
        return "sample";
    }
    public String getSampleField() {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@interface CustomAnnotationwe {
    String name();
    String value();
}
