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


public class ReflectionIsSynthetic {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        new IsSynthetic();
        try {
            Class zqp1 = IsSynthetic.class;
            Class zqp2 = int.class;
            Class zqp3 = Class.forName("ReflectionIsSynthetic$1");
            if (!zqp2.isSynthetic()) {
                if (!zqp1.isSynthetic()) {
                    if (zqp3.isSynthetic()) {
                        result = 0;
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            result = -1;
        }
        System.out.println(result);
    }
    private static class IsSynthetic {
    }
}