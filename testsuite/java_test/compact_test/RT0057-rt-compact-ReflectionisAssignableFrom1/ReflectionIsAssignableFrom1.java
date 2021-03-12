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


class AssignableFrom1 {
}
class AssignableFrom1_a extends AssignableFrom1 {
}
interface AssignableFromTest1 {
}
interface AssignableFromTest1_a extends AssignableFromTest1 {
}
public class ReflectionIsAssignableFrom1 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("AssignableFrom1");
            Class clazz2 = Class.forName("AssignableFrom1_a");
            Class clazz3 = Class.forName("AssignableFromTest1");
            Class clazz4 = Class.forName("AssignableFromTest1_a");
            if (clazz1.isAssignableFrom(clazz2) && !clazz2.isAssignableFrom(clazz1) && clazz3.isAssignableFrom(clazz4)
                    && !clazz4.isAssignableFrom(clazz3)) {
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
