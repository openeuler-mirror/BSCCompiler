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
public class MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException {
    private static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException().run());
    }
    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException_1();
        } catch (Exception e) {
            e.printStackTrace();
            MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 20;
        }
        try {
            result = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException_2();
        } catch (Exception e) {
            e.printStackTrace();
            MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 20;
        }
        if (result == 4 && MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res == 74) {
            result = 0;
        }
        return result;
    }
    private int MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException_1() throws NoSuchMethodException {
        //   <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
        int result1 = 4; /*STATUS_FAILED*/
        Method m = SampleClass_h6.class.getMethod("sampleMethod");
        try {
            CustomAnnotations_h11[] tp = m.getAnnotationsByType(CustomAnnotations_h11.class);
            if (tp.length == 1 &&
                    tp[0].toString().equals("@CustomAnnotations_h11(name=sampleMethod, value=Sample Method Annotation)")) {
                MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 10;
            }
        } catch (Exception e) {
            MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 15;
        }
        return result1;
    }
    private int MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException_2() throws NoSuchMethodException {
        //   NullPointerException - if the given annotation class is null
        //   <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
        int result1 = 4; /*STATUS_FAILED*/
        Method m = SampleClass_h6.class.getMethod("sampleMethod");
        try {
            m.getAnnotationsByType(null);
            MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 10;
        } catch (NullPointerException e) {
            MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = MethodExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 15;
        }
        return result1;
    }
}
@CustomAnnotations_h11(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass_h6 {
    private String sampleField;
    @CustomAnnotations_h11(name = "sampleMethod", value = "Sample Method Annotation")
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
@interface CustomAnnotations_h11 {
    String name();
    String value();
}
