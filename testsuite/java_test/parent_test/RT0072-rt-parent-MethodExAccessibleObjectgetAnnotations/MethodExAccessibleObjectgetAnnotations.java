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


import java.lang.annotation.Annotation;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Method;
public class MethodExAccessibleObjectgetAnnotations {
    private static int res = 99;
    public static void main(String[] args) {
        System.out.println(new MethodExAccessibleObjectgetAnnotations().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectsetAccessibleSecurityException1();
        } catch (Exception e) {
            e.printStackTrace();
            MethodExAccessibleObjectgetAnnotations.res = MethodExAccessibleObjectgetAnnotations.res - 20;
        }
        if (result == 4 && MethodExAccessibleObjectgetAnnotations.res == 89) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectsetAccessibleSecurityException1() throws NoSuchMethodException {
        //  Annotation[] getAnnotations()
        int result1 = 4;
        Method method = SampleClassH4.class.getMethod("sampleMethod");
        try {
            Annotation[] tp = method.getAnnotations();
            if (tp.length == 1 &&
                    tp[0].toString().equals("@CustomAnnotationsH9(name=sampleMethod, value=Sample Method Annotation)")) {
                MethodExAccessibleObjectgetAnnotations.res = MethodExAccessibleObjectgetAnnotations.res - 10;
            }
        } catch (Exception e) {
            MethodExAccessibleObjectgetAnnotations.res = MethodExAccessibleObjectgetAnnotations.res - 15;
        }
        return result1;
    }
}
@CustomAnnotationsH9(name = "SampleClass", value = "Sample Class Annotation")
class SampleClassH4 {
    private String sampleField;
    @CustomAnnotationsH9(name = "sampleMethod", value = "Sample Method Annotation")
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
@interface CustomAnnotationsH9 {
    String name();
    String value();
}
