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
import java.lang.reflect.Executable;
public class ConstructorExExecutablegetExceptionTypes {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = constructorExExecutablegetExceptionTypes1();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (result == 4 && ConstructorExExecutablegetExceptionTypes.res == 97) {
            result = 0;
        }
        return result;
    }
    private static int constructorExExecutablegetExceptionTypes1() {
        int result1 = 4;
        try {
            // abstract Class[]<?> getExceptionTypes()
            Executable sampleExecutable = SampleClass12.class.getConstructor(String.class);
            Class<?>[] vlClass = sampleExecutable.getExceptionTypes();
            if (vlClass.length == 0 && vlClass.getClass().toString().equals("class [Ljava.lang.Class;")) {
                ConstructorExExecutablegetExceptionTypes.res = ConstructorExExecutablegetExceptionTypes.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException e1) {
            String eMsg = e1.toString();
        }
        return result1;
    }
}
@CustomAnnotations12(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass12 {
    private String sampleField;
    @CustomAnnotations12(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass12(String str) {
        this.sampleField = str;
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
@interface CustomAnnotations12 {
    String name();
    String value();
}