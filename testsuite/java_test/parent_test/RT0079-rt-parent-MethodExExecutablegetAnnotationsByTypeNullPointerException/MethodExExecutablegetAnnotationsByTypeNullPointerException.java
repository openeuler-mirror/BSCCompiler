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


import java.lang.reflect.Method;
public class MethodExExecutablegetAnnotationsByTypeNullPointerException {
    static int res = 99;
    public static void main(String[] argv) {
        System.out.println(new MethodExExecutablegetAnnotationsByTypeNullPointerException().run());
    }
    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = MethodExExecutablegetAnnotationsByTypeNullPointerException_1();
        } catch (Exception e) {
            MethodExExecutablegetAnnotationsByTypeNullPointerException.res = MethodExExecutablegetAnnotationsByTypeNullPointerException.res - 20;
        }
        try {
            result = MethodExExecutablegetAnnotationsByTypeNullPointerException_2();
        } catch (Exception e) {
            MethodExExecutablegetAnnotationsByTypeNullPointerException.res = MethodExExecutablegetAnnotationsByTypeNullPointerException.res - 20;
        }
        if (result == 4 && MethodExExecutablegetAnnotationsByTypeNullPointerException.res == 74) {
            result = 0;
        }
        return result;
    }
    private int MethodExExecutablegetAnnotationsByTypeNullPointerException_1() throws NoSuchMethodException {
        //  <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
        int result1 = 4;
        Method m1 = SampleClass_h1.class.getDeclaredMethod("sampleMethod");
        try {
            CustomAnnotations_h5[] tp = m1.getAnnotationsByType(CustomAnnotations_h5.class);
            if(tp.getClass().toString().equals("class [LCustomAnnotations_h5;")){
                MethodExExecutablegetAnnotationsByTypeNullPointerException.res = MethodExExecutablegetAnnotationsByTypeNullPointerException.res - 10;
            }
        } catch (Exception e) {
            MethodExExecutablegetAnnotationsByTypeNullPointerException.res = MethodExExecutablegetAnnotationsByTypeNullPointerException.res - 15;
        }
        return result1;
    }
    private int MethodExExecutablegetAnnotationsByTypeNullPointerException_2() throws NoSuchMethodException {
        //  NullPointerException - if the given annotation class is null
        //  <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
        int result1 = 4;
        Method m1 = SampleClass_h1.class.getDeclaredMethod("sampleMethod");
        try {
            CustomAnnotations_h5[] tp = m1.getAnnotationsByType(null);
            MethodExExecutablegetAnnotationsByTypeNullPointerException.res = MethodExExecutablegetAnnotationsByTypeNullPointerException.res - 10;
        } catch (NullPointerException e) {
            MethodExExecutablegetAnnotationsByTypeNullPointerException.res = MethodExExecutablegetAnnotationsByTypeNullPointerException.res - 15;
        }
        return result1;
    }
}
@CustomAnnotations_h5(name="SampleClass",  value = "Sample Class Annotation")
class SampleClass_h1 {
    private String sampleField;
    @CustomAnnotations_h5(name="sampleMethod",  value = "Sample Method Annotation")
    public String sampleMethod(){
        return "sample";
    }
    public String getSampleField() {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@interface CustomAnnotations_h5 {
    String name();
    String value();
}
