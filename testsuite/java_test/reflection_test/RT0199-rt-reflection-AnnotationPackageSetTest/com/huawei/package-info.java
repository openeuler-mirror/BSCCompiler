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

@AnnoA(intA = Integer.MAX_VALUE, byteA = Byte.MAX_VALUE, charA = Character.MAX_VALUE, doubleA = Double.NaN,
        booleanA = true, longA = Long.MAX_VALUE, floatA = Float.NaN, shortA = Short.MAX_VALUE,
        intAA = {1,2}, byteAA = {0}, charAA = {' '}, doubleAA = {Double.NaN, Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY},
        booleanAA = {true}, longAA = {Long.MAX_VALUE}, floatAA = {Float.NaN, Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY}, shortAA = {0},
        stringA = "", stringAA = "", classA = Thread.class, classAA = Thread.class, stateA = Thread.State.BLOCKED,
        stateAA = Thread.State.NEW, annoBA = @AnnoB, annoBAA = {@AnnoB, @AnnoB})
package com.huawei;
class PackageInfo{
    public void say(){
        System.out.println("hello");
    }
}
class PackageInfoGeneric<T extends Throwable>{
    private T obj;
    public void setObj(T obj) {
        this.obj = obj;
    }
    public void say(){
        System.out.println("hello "+obj);
    }
}
interface PackageInfoInterface {
    void test();
}
class PackageInfoConst {
    public static final String MESSAGE="Annotation Study";
}