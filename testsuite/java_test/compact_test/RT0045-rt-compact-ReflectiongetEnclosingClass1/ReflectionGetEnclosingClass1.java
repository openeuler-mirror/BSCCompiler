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


public class ReflectionGetEnclosingClass1 {
    public static void main(String[] args) {
        Class clazz1 = (new GetEnclosingClassTest1()).test1().getClass();
        Class clazz2 = (new GetEnclosingClassTest1()).test2().getClass();
        if (clazz1.getEnclosingClass().getName().equals("ReflectionGetEnclosingClass1$GetEnclosingClassTest1")
                && clazz2.getEnclosingClass().getName().equals("ReflectionGetEnclosingClass1$GetEnclosingClassTest1")) {
            System.out.println(0);
        }
    }
    public static class GetEnclosingClassTest1 {
        public Object test1() {
            class classA {
            }
            return new classA();
        }
        Object test2() {
            class classB {
            }
            return new classB();
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
