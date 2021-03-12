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


import java.lang.annotation.*;
import java.lang.reflect.Method;
public class MethodGetDefaultValue {
    public static void main(String[] argv) {
        int result = 2/* STATUS_FAILED*/
;
        try {
            result = MethodGetDefaultValueTypeNotPresent_1();
        } catch (Exception e) {
            result = 3;
        }
        System.out.println(result);
    }
    public static int MethodGetDefaultValueTypeNotPresent_1() {
        try {
            Method m = MyTargetTest13.class.getMethod("MyTargetTest_1");
            Object a = m.getDefaultValue();
        } catch (TypeNotPresentException e) {
            return 3;
        } catch (NoSuchMethodException e) {
            return 4;
        }
        return 0;
    }
    @Target(ElementType.TYPE)
    @Retention(RetentionPolicy.RUNTIME)
    public @interface MyTarget {
        Class style() default String.class;
    }
    @MyTarget()
    class MyTargetTest13 {
        public void MyTargetTest_1() {
            System.out.println("This is Example:hello world");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
