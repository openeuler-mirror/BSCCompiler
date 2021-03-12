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


class AssignableFromNullPointerException {
}
public class ReflectionIsAssignableFromNullPointerException {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        try {
            Class zqp1 = Class.forName("AssignableFromNullPointerException");
            zqp1.isAssignableFrom(null);
            result = -1;
        } catch (ClassNotFoundException e1) {
            result = -1;
        } catch (NullPointerException e2) {
            result = 0;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
