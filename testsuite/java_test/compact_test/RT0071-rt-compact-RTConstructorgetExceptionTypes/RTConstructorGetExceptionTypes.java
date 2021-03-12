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


import java.lang.reflect.Constructor;
class ConstructorGetExceptionTypes {
    ConstructorGetExceptionTypes() throws ExceptionInInitializerError, InstantiationException {
    }
}
public class RTConstructorGetExceptionTypes {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetExceptionTypes");
            Constructor cons = cls.getDeclaredConstructor();
            Class<?>[] exClass = cons.getExceptionTypes();
            if (exClass.length == 2) {
                System.out.println(0);
                return;
            }
            System.out.println(1);
            return;
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(3);
            return;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
