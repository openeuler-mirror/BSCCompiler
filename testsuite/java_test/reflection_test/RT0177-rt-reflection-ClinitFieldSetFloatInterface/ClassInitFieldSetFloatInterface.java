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
public class ClassInitFieldSetFloatInterface {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiFloat");
            if (result.toString().compareTo("") == 0) {
                f.setFloat(null, 0.654f);
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
    String aSuper = ClassInitFieldSetFloatInterface.result.append("Super").toString();
}
@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldSetFloatInterface.result.append("One").toString();
    float hiFloat = 0.25f;
}
interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldSetFloatInterface.result.append("Two").toString();
}
@interface A {
    String aA = ClassInitFieldSetFloatInterface.result.append("Annotation").toString();
}