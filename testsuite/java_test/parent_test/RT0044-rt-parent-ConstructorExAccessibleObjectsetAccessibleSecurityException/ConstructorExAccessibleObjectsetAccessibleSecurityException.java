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


import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.AccessibleObject;
public class ConstructorExAccessibleObjectsetAccessibleSecurityException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        result = constructorExAccessibleObjectsetAccessibleSecurityExceptionTest();
        constructorExAccessibleObjectsetAccessibleSecurityExceptionTest2();
        if (result == 4 && ConstructorExAccessibleObjectsetAccessibleSecurityException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int constructorExAccessibleObjectsetAccessibleSecurityExceptionTest() {
        int result1 = 4;
        try {
            // void setAccessible(boolean flag)
            AccessibleObject sampleconstructor = SampleClass8.class.getConstructor(String.class);
            sampleconstructor.setAccessible(true);
            boolean value = sampleconstructor.isAccessible();
            if (value) {
                // System.out.println(b);
                ConstructorExAccessibleObjectsetAccessibleSecurityException.res = ConstructorExAccessibleObjectsetAccessibleSecurityException.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException e) {
            String eMsg = e.toString();
        }
        return result1;
    }
    private static void constructorExAccessibleObjectsetAccessibleSecurityExceptionTest2() {
        try {
            // static void setAccessible(AccessibleObject[] array, boolean flag)
            AccessibleObject[] sampleconstructorlist = SampleClass8.class.getConstructors();
            AccessibleObject.setAccessible(sampleconstructorlist, true);
            boolean value = sampleconstructorlist[0].isAccessible();
            if (value) {
                ConstructorExAccessibleObjectsetAccessibleSecurityException.res = ConstructorExAccessibleObjectsetAccessibleSecurityException.res - 2;
            }
        } catch (SecurityException e) {
            String eMsg = e.toString();
        }
    }
}
@CustomAnnotations8(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass8 {
    private String sampleField;
    @CustomAnnotations8(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass8(String str) {
    }
    public String getSampleField() {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE, ElementType.METHOD, ElementType.CONSTRUCTOR})
@interface CustomAnnotations8 {
    String name();
    String value();
}
