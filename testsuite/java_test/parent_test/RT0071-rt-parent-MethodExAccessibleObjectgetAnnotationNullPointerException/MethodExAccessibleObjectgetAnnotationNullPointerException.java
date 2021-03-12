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
public class MethodExAccessibleObjectgetAnnotationNullPointerException {
    static int res = 100;
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
            result = methodExAccessibleObjectgetAnnotationNullPointerException1();
        } catch (Exception e) {
            MethodExAccessibleObjectgetAnnotationNullPointerException.res = MethodExAccessibleObjectgetAnnotationNullPointerException.res - 10;
        }
        try {
            result = methodExAccessibleObjectgetAnnotationNullPointerException2();
        } catch (Exception e) {
            MethodExAccessibleObjectgetAnnotationNullPointerException.res = MethodExAccessibleObjectgetAnnotationNullPointerException.res - 10;
        }
        if (result == 4 && MethodExAccessibleObjectgetAnnotationNullPointerException.res == 75) {
            result = 0;
        }
        return result;
    }
    private static int methodExAccessibleObjectgetAnnotationNullPointerException1() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // NullPointerException - If the specified object is empty.
        // <T extends Annotation> T getAnnotation(Class<T> annotationClass)
        Method sampleMethod = MethodClass9.class.getMethod("sampleMethod");
        try {
            sampleMethod.getAnnotation(null);
            MethodExAccessibleObjectgetAnnotationNullPointerException.res -= 10;
        } catch (NullPointerException e1) {
            MethodExAccessibleObjectgetAnnotationNullPointerException.res = MethodExAccessibleObjectgetAnnotationNullPointerException.res - 15;
        }
        return result1;
    }
    private static int methodExAccessibleObjectgetAnnotationNullPointerException2() throws NoSuchMethodException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // <T extends Annotation> T getAnnotation(Class<T> annotationClass)
        Method sampleMethod = MethodClass9.class.getMethod("sampleMethod");
        try {
            MethodAnnotation get1 = sampleMethod.getAnnotation(MethodAnnotation.class);
            if (get1.toString().equals("@MethodAnnotation(name=sampleMethod, value=Sample Method Annotation)")) {
                MethodExAccessibleObjectgetAnnotationNullPointerException.res -= 10;
            }
        } catch (NullPointerException e1) {
            MethodExAccessibleObjectgetAnnotationNullPointerException.res = MethodExAccessibleObjectgetAnnotationNullPointerException.res - 15;
        }
        return result1;
    }
}
@MethodAnnotation(name = "SampleClass", value = "Sample Class Annotation")
class MethodClass9 {
    private String sampleField;
    @MethodAnnotation(name = "sampleMethod", value = "Sample Method Annotation")
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
@interface MethodAnnotation {
    String name();
    String value();
}