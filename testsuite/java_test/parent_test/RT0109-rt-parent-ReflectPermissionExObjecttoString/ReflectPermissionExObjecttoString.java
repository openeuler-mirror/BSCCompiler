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
public class ReflectPermissionExObjecttoString {
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
            result = reflectPermissionExObjecttoString1();
        } catch (Exception e) {
            ReflectPermissionExObjecttoString.res = ReflectPermissionExObjecttoString.res - 20;
        }
        if (result == 4 && ReflectPermissionExObjecttoString.res == 89) {
            result = 0;
        }
        return result;
    }
    private static int reflectPermissionExObjecttoString1() {
        int result1 = 4; /*STATUS_FAILED*/
        // String toString()
        String name = "name";
        String actions = null;
        ReflectPermission rp = new ReflectPermission(name, actions);
        try {
            String str1 = rp.toString();
            if (str1.contains("java.lang.reflect.ReflectPermission")) {
                ReflectPermissionExObjecttoString.res = ReflectPermissionExObjecttoString.res - 10;
            }
        } catch (IllegalMonitorStateException e) {
            ReflectPermissionExObjecttoString.res = ReflectPermissionExObjecttoString.res - 1;
        }
        return result1;
    }
}
