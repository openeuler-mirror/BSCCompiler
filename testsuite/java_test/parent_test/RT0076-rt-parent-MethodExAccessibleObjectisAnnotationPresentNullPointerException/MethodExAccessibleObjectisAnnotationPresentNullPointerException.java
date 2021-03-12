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


import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;
public class MethodExAccessibleObjectisAnnotationPresentNullPointerException {
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
            result = methodExAccessibleObjectisAnnotationPresentNullPointerException1();
        } catch (Exception e) {
            MethodExAccessibleObjectisAnnotationPresentNullPointerException.res = MethodExAccessibleObjectisAnnotationPresentNullPointerException.res - 20;
        }
        try {
            result = methodExAccessibleObjectisAnnotationPresentNullPointerException2();
        } catch (Exception e) {
            MethodExAccessibleObjectisAnnotationPresentNullPointerException.res = MethodExAccessibleObjectisAnnotationPresentNullPointerException.res - 20;
        }
        if (result == 4 && MethodExAccessibleObjectisAnnotationPresentNullPointerException.res == 68) {
            result = 0;
        }
        return result;
    }
    private static int methodExAccessibleObjectisAnnotationPresentNullPointerException1() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - If the specified object is empty.
        Method sampleMethod = MethodClass8.class.getMethod("sampleMethod");
        try {
            sampleMethod.isAnnotationPresent(null);
            MethodExAccessibleObjectisAnnotationPresentNullPointerException.res = MethodExAccessibleObjectisAnnotationPresentNullPointerException.res - 10;
        } catch (NullPointerException e1) {
            MethodExAccessibleObjectisAnnotationPresentNullPointerException.res = MethodExAccessibleObjectisAnnotationPresentNullPointerException.res - 1;
        }
        return result1;
    }
    private static int methodExAccessibleObjectisAnnotationPresentNullPointerException2() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - If the specified object is empty.
        // boolean isAnnotationPresent(Class<? extends Annotation> annotationClass)
        Method sampleMethod = MethodClass8.class.getMethod("sampleMethod");
        try {
            sampleMethod.isAnnotationPresent(CustomAnnotationH100.class);
            MethodExAccessibleObjectisAnnotationPresentNullPointerException.res = MethodExAccessibleObjectisAnnotationPresentNullPointerException.res - 30;
        } catch (NullPointerException e1) {
            MethodExAccessibleObjectisAnnotationPresentNullPointerException.res = MethodExAccessibleObjectisAnnotationPresentNullPointerException.res - 5;
        }
        return result1;
    }
}
@CustomAnnotationH100(name = "SampleClass", value = "Sample Class Annotation")
class MethodClass8 {
    private String sampleField;
    @CustomAnnotationH100(name = "sampleMethod", value = "Sample Method Annotation")
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
@Retention(RetentionPolicy.RUNTIME)
@interface CustomAnnotationH100 {
    String name();
    String value();
}
