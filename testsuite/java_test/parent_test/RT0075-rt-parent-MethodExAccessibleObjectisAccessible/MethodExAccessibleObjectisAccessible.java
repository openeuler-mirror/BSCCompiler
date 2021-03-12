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
public class MethodExAccessibleObjectisAccessible {
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
            result = methodExAccessibleObjectisAccessible1();
        } catch (Exception e) {
            MethodExAccessibleObjectisAccessible.res = MethodExAccessibleObjectisAccessible.res - 10;
        }
        if (result == 4 && MethodExAccessibleObjectisAccessible.res == 98) {
            result = 0;
        }
        return result;
    }
    private static int methodExAccessibleObjectisAccessible1() throws NoSuchMethodException, SecurityException, NoSuchFieldException {
        int result1 = 4; /*STATUS_FAILED*/
        // boolean isAccessible()
        Method sampleMethod = MethodClass11.class.getMethod("sampleMethod");
        if (sampleMethod.isAccessible()) {
            MethodExAccessibleObjectisAccessible.res = MethodExAccessibleObjectisAccessible.res - 10;
        } else {
            MethodExAccessibleObjectisAccessible.res = MethodExAccessibleObjectisAccessible.res - 1;
        }
        return result1;
    }
}
@CustomAnnotationn(name = "SampleClass", value = "Sample Class Annotation")
class MethodClass11 {
    private String sampleField;
    @CustomAnnotationn(name = "sampleMethod", value = "Sample Class Annotation")
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
@interface CustomAnnotationn {
    String name();
    String value();
}
