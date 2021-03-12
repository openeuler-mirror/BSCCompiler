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


import java.lang.reflect.Field;
public class ThreadGroupAllowThreadSuspensionTest {
    public static void main(String[] args) {
        ThreadGroup tg = new ThreadGroup("group");
        tg.allowThreadSuspension(true);
        try {
            if (getValue(tg, "vmAllowSuspension").toString() != "true") {
                System.out.println(2);
                return;
            }
        } catch (Throwable e) {
            System.out.println(2);
            return;
        }
        tg.allowThreadSuspension(false);
        try {
            if (getValue(tg, "vmAllowSuspension").toString() != "false") {
                System.out.println(2);
                return;
            }
        } catch (Throwable e) {
            System.out.println(2);
            return;
        }
        System.out.println(0);
    }
    // Use reflect to get the value of private field of ThreadGroup
    public static Object getValue(Object instance, String fieldName) throws IllegalAccessException, NoSuchFieldException {
        Field field = getField(instance.getClass(), fieldName);
        field.setAccessible(true);
        return field.get(instance);
    }
    public static Field getField(Class thisClass, String fieldName) throws NoSuchFieldException {
        if (fieldName == null) {
            throw new NoSuchFieldException("Error field !");
        }
        Field field = thisClass.getDeclaredField(fieldName);
        return field;
    }
}