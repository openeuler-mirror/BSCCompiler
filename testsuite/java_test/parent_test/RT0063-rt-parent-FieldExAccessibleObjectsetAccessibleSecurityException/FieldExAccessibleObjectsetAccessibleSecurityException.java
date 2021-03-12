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
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectsetAccessibleSecurityException {
    static int res = 99;
    public static void main(String[] args) {
        System.out.println(new FieldExAccessibleObjectsetAccessibleSecurityException().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectsetAccessibleSecurityException1();
        } catch (Exception e) {
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 20;
        }
        try {
            result = fieldExAccessibleObjectsetAccessibleSecurityException2();
        } catch (Exception e) {
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectsetAccessibleSecurityException.res == 59) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectsetAccessibleSecurityException1() throws NoSuchFieldException {
        //  void setAccessible(boolean flag)
        int result1 = 4;
        Field f1 = SampleClassFieldH4.class.getDeclaredField("id");
        try {
            f1.setAccessible(false);
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 10;
        } catch (SecurityException e) {
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 15;
        }
        try {
            f1.setAccessible(true);
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 10;
        } catch (SecurityException e) {
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 15;
        }
        return result1;
    }
    private int fieldExAccessibleObjectsetAccessibleSecurityException2() throws NoSuchFieldException {
        //  static void setAccessible(AccessibleObject[] array, boolean flag)
        int result2 = 4;
        Constructor[] test = AccessibleObject.class.getDeclaredConstructors();
        try {
            AccessibleObject.setAccessible(test, false);
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 10;
        } catch (SecurityException e) {
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 15;
        }
        try {
            AccessibleObject.setAccessible(test, true);
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 10;
        } catch (SecurityException e) {
            FieldExAccessibleObjectsetAccessibleSecurityException.res = FieldExAccessibleObjectsetAccessibleSecurityException.res - 15;
        }
        return result2;
    }
}
class SampleClassFieldH4 {
    @CustomAnnotationsH4(name = "id")
    String id;
}
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@interface CustomAnnotationsH4 {
    String name();
}
