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


import java.lang.reflect.ReflectPermission;
public class ReflectPermissionExObjectgetClass {
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
            result = reflectPermissionExObjectgetClass1();
        } catch (Exception e) {
            ReflectPermissionExObjectgetClass.res = ReflectPermissionExObjectgetClass.res - 20;
        }
        if (result == 4 && ReflectPermissionExObjectgetClass.res == 89) {
            result = 0;
        }
        return result;
    }
    private static int reflectPermissionExObjectgetClass1() throws ClassNotFoundException {
        //  final Class<?> getClass()
        int result1 = 4; /*STATUS_FAILED*/
        String name = "name";
        String actions = null;
        ReflectPermission rp = new ReflectPermission(name, actions);
        try {
            Class<? extends ReflectPermission> cls1 = rp.getClass();
            if (cls1.toString().equals("class java.lang.reflect.ReflectPermission")) {
                ReflectPermissionExObjectgetClass.res = ReflectPermissionExObjectgetClass.res - 10;
            }
        } catch (IllegalMonitorStateException e) {
            ReflectPermissionExObjectgetClass.res = ReflectPermissionExObjectgetClass.res - 1;
        }
        return result1;
    }
}