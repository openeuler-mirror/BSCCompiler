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


public class ReflectionIsMemberClass {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        class IsMemberClass_a {
        }
        try {
            Class zqp1 = IsMemberClass.class;
            Class zqp2 = Class.forName("ReflectionIsMemberClass");
            Class zqp3 = IsMemberClass_a.class;
            Class zqp4 = IsMemberClass_b.class;
            Class zqp5 = (new IsMemberClass_b() {
            }).getClass();
            if (!zqp2.isMemberClass()) {
                if (!zqp3.isMemberClass()) {
                    if (zqp4.isMemberClass()) {
                        if (!zqp5.isMemberClass()) {
                            if (zqp1.isMemberClass()) {
                                result = 0;
                            }
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            result = -1;
        }
        System.out.println(result);
    }
    class IsMemberClass {
    }
    static class IsMemberClass_b {
    }
}