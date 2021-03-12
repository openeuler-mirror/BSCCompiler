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
public class ClassInitFieldOtherMethodInterface {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hi");
            f.equals(f);
            f.getAnnotation(A.class);
            f.getAnnotationsByType(A.class);
            f.getDeclaredAnnotations();
            f.getDeclaringClass();
            f.getGenericType();
            f.getModifiers();
            f.getName();
            f.getType();
            f.hashCode();
            f.isEnumConstant();
            f.isSynthetic();
            f.toGenericString();
            f.toString();
        } catch (Exception e) {
            System.out.println(e);
        }
        if(result.toString().compareTo("") == 0) {
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
}
interface SuperInterface{
    String aSuper = ClassInitFieldOtherMethodInterface.result.append("Super").toString();
}
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldOtherMethodInterface.result.append("One").toString();
    @A
    short hi = 14;
}
interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldOtherMethodInterface.result.append("Two").toString();
}
@interface A {
    String aA = ClassInitFieldOtherMethodInterface.result.append("Annotation").toString();
}