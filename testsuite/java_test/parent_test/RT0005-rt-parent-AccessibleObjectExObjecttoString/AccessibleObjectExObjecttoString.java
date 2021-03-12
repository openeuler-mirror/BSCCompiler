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


import java.lang.reflect.AccessibleObject;
public class AccessibleObjectExObjecttoString {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new AccessibleObjectExObjecttoString().run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = accessibleObjectExObjecttoString1();
        } catch (Exception e) {
            AccessibleObjectExObjecttoString.res = AccessibleObjectExObjecttoString.res - 20;
        }
        if (result == 4 && AccessibleObjectExObjecttoString.res == 89) {
            result = 0;
        }
        return result;
    }
    private int accessibleObjectExObjecttoString1() throws NoSuchFieldException, SecurityException {
        int result1 = 4; /*STATUS_FAILED*/
//         String toString()
        AccessibleObject sampleField1 = AccessibleObjectExObjecttoString.class.getDeclaredField("res");
        String px1 = sampleField1.toString();
//         System.out.println(px1);
        if (px1.equals("static int AccessibleObjectExObjecttoString.res")) {
            AccessibleObjectExObjecttoString.res = AccessibleObjectExObjecttoString.res - 10;
        }
        return result1;
    }
}