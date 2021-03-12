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


import java.lang.reflect.Field;
public class ClassInitFieldGetIntInterface {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiInt");
            if (result.toString().compareTo("") == 0) {
                f.getInt(null);
            }
        } catch (Exception e) {
            System.out.println(e);
        }
        if (result.toString().compareTo("One") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}
interface SuperInterface{
    String aSuper = ClassInitFieldGetIntInterface.result.append("Super").toString();
}
@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldGetIntInterface.result.append("One").toString();
    int hiInt = 125;
}
interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldGetIntInterface.result.append("Two").toString();
}
@interface A {
    String aA = ClassInitFieldGetIntInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
