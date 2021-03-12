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


class GetEnclosingClassTest2 {
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
public class ReflectionGetEnclosingClass2 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetEnclosingClassTest2");
            Class clazz2 = (new GetEnclosingClassTest2()).test2().getClass();
            if (clazz1.getEnclosingClass() == null
                    && clazz2.getEnclosingClass().getName().equals("GetEnclosingClassTest2")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
