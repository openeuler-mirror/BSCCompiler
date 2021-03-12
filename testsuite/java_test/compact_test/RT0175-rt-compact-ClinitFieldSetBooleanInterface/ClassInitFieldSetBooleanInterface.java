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
public class ClassInitFieldSetBooleanInterface {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiBoolean");
            if (result.toString().compareTo("") == 0) {
                f.setBoolean(null, true);
            }
        }catch (IllegalAccessException e){
            result.append("IllegalAccessException");
        }catch (Exception e) {
            System.out.println(e);
        }
        if (result.toString().compareTo("OneIllegalAccessException") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}
interface SuperInterface{
    String aSuper = ClassInitFieldSetBooleanInterface.result.append("Super").toString();
}
@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldSetBooleanInterface.result.append("One").toString();
    boolean hiBoolean = false;
}
interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldSetBooleanInterface.result.append("Two").toString();
}
@interface A {
    String aA = ClassInitFieldSetBooleanInterface.result.append("Annotation").toString();
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
