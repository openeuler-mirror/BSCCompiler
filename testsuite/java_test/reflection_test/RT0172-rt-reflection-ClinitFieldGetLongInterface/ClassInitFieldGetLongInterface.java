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
public class ClassInitFieldGetLongInterface {
    static StringBuffer result = new StringBuffer("");
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("OneInterface", false, OneInterface.class.getClassLoader());
            Field f = clazz.getField("hiLong");
            if (result.toString().compareTo("") == 0) {
                f.getLong(null);
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
    String aSuper = ClassInitFieldGetLongInterface.result.append("Super").toString();
}
@A
interface OneInterface extends SuperInterface{
    String aOne = ClassInitFieldGetLongInterface.result.append("One").toString();
    long hiLong = 4859l;
}
interface TwoInterface extends OneInterface{
    String aTwo = ClassInitFieldGetLongInterface.result.append("Two").toString();
}
@interface A {
    String aA = ClassInitFieldGetLongInterface.result.append("Annotation").toString();
}