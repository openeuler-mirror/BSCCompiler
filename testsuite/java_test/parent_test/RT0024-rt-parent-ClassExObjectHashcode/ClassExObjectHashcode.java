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


import java.io.PrintStream;
public class ClassExObjectHashcode {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    /**
     * main test fun
     *
     * @return status code
    */

    public static int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = classExObjectHashcode();
        } catch (Exception e) {
            ClassExObjectHashcode.res = ClassExObjectHashcode.res - 10;
        }
        if (result == 4 && ClassExObjectHashcode.res == 89) {
            result = 0;
        }
        return result;
    }
    private static int classExObjectHashcode() {
        // int hashCode()
        int result1 = 4; /*STATUS_FAILED*/
        Class<?> cal = ClassExObjectHashcode.class.getClass();
        try {
            int hashcode = cal.hashCode();
            ClassExObjectHashcode.res = ClassExObjectHashcode.res - 10;
        } catch (IllegalArgumentException e1) {
            ClassExObjectHashcode.res = ClassExObjectHashcode.res - 1;
        }
        return result1;
    }
}
