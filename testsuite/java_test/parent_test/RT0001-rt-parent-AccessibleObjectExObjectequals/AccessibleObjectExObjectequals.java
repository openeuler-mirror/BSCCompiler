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
import java.lang.reflect.*;
public class AccessibleObjectExObjectequals {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new AccessibleObjectExObjectequals().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = accessibleObjectExObjectequals1();
        } catch (Exception e) {
            AccessibleObjectExObjectequals.res = AccessibleObjectExObjectequals.res - 20;
        }
        if (result == 4 && AccessibleObjectExObjectequals.res == 89) {
            result = 0;
        }
        return result;
    }
    private int accessibleObjectExObjectequals1() throws NoSuchFieldException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
        // boolean equals(Object obj)
        AccessibleObject sampleField2 = AccessibleClass1.class.getDeclaredField("sampleField");
        AccessibleObject sampleField1 = AccessibleClass1.class.getDeclaredField("sampleField");
        if (sampleField2.equals(sampleField1)) {
            AccessibleObjectExObjectequals.res = AccessibleObjectExObjectequals.res - 10;
        } else {
            AccessibleObjectExObjectequals.res = AccessibleObjectExObjectequals.res - 1;
        }
        return result1;
    }
}
@AccessibleAnnotation1(name = "SampleClass", value = "Sample Class Annotation")
class AccessibleClass1 {
    private String sampleField;
    @AccessibleAnnotation1(name = "sampleMethod", value = "Sample Method Annotation")
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
@interface AccessibleAnnotation1 {
    String name();
    String value();
}