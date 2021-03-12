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


interface NewInstance2 {
}
class NewInstance2_a {
    private NewInstance2_a() {
    }
}
public class ReflectionNewInstanceInstantiationExceptionIllegalAccessException {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        try {
            Class zqp1 = Class.forName("NewInstance2");
            Object zhu1 = zqp1.newInstance();
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            result = -1;
        } catch (InstantiationException e1) {
            try {
                Class zqp2 = Class.forName("NewInstance2_a");
                Object zhu2 = zqp2.newInstance();
            } catch (ClassNotFoundException e3) {
                System.err.println(e3);
                result = -1;
            } catch (InstantiationException e4) {
                System.err.println(e4);
                result = -1;
            } catch (IllegalAccessException e5) {
                result = 0;
            }
        } catch (IllegalAccessException e2) {
            System.err.println(e2);
            result = -1;
        }
        System.out.println(result);
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
