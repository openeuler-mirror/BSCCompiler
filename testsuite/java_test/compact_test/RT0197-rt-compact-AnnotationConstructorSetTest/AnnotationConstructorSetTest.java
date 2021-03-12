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

import java.util.Arrays;
public class AnnotationConstructorSetTest {
    static int[] passCnt;
    public static void main(String[] args) throws NoSuchMethodException {
//        System.out.println(Test.class.getDeclaredMethod("a"));
        AnnoA anno = Test.class.getDeclaredConstructor(Object[].class).getDeclaredAnnotation(AnnoA.class);
        passCnt = new int[24];
        // 基本数据类型
        int i = 0;
        passCnt[i++] = anno.intA() == Integer.MAX_VALUE ? 1: 0;
        passCnt[i++] += anno.byteA() == Byte.MAX_VALUE ? 1: 0;
        passCnt[i++] += anno.charA() == Character.MAX_VALUE ? 1: 0;
        passCnt[i++] += Double.isNaN(anno.doubleA()) ? 1: 0;
        passCnt[i++] += anno.booleanA() ? 1: 0;
        passCnt[i++] += anno.longA() == Long.MAX_VALUE ? 1: 0;
        passCnt[i++] += Float.isNaN(anno.floatA())? 1: 0;
        passCnt[i++] += anno.shortA() == Short.MAX_VALUE ? 1: 0;
        //enum, string, annotation, class
        passCnt[i++] += anno.stateA() == Thread.State.BLOCKED ? 1: 0;
        passCnt[i++] += anno.stringA().compareTo("") == 0 ? 1: 0;
        passCnt[i++] += anno.classA() == Thread.class ? 1: 0;
        passCnt[i++] += Arrays.toString(anno.intAA()).compareTo("[1, 2]") == 0 ? 1: 0;
        //基本类型数组
        passCnt[i++] += (anno.byteAA().length == 1 && anno.byteAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.charAA().length == 1 && anno.charAA()[0] == ' ') ? 1: 0;
        passCnt[i++] += (anno.doubleAA().length == 3 && Double.isNaN(anno.doubleAA()[0]) && Double.isInfinite(anno.doubleAA()[1]) && Double.isInfinite(anno.doubleAA()[2]))? 1: 0;
        passCnt[i++] += (anno.booleanAA().length == 1 && anno.booleanAA()[0]) ? 1: 0;
        passCnt[i++] += (anno.longAA().length == 1 && anno.longAA()[0] == Long.MAX_VALUE) ? 1: 0;
        passCnt[i++] += (anno.floatAA().length == 3 && Float.isNaN(anno.floatAA()[0]) && Float.isInfinite(anno.floatAA()[1]) && Float.isInfinite(anno.floatAA()[2])) ? 1: 0;
        passCnt[i++] += (anno.shortAA().length == 1 && anno.shortAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.stringAA().length == 1 && anno.stringAA()[0].compareTo("")==0) ? 1: 0;
        passCnt[i++] += (anno.classAA().length == 1 && anno.classAA()[0] == Thread.class)? 1: 0;
        passCnt[i++] += (anno.stateAA().length == 1 && anno.stateAA()[0] == Thread.State.NEW) ? 1: 0;
        passCnt[i++] += anno.annoBA().toString().compareTo("@AnnoB(intB=999)")==0 ? 1: 0;
        passCnt[i++] += Arrays.toString(anno.annoBAA()).compareTo("[@AnnoB(intB=999), @AnnoB(intB=999)]") == 0 ? 1: 0;
        System.out.println(Arrays.toString(passCnt).compareTo("[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]"));
    }
}
class Test{
    @AnnoA(intA = Integer.MAX_VALUE, byteA = Byte.MAX_VALUE, charA = Character.MAX_VALUE, doubleA = Double.NaN,
            booleanA = true, longA = Long.MAX_VALUE, floatA = Float.NaN, shortA = Short.MAX_VALUE,
            intAA = {1,2}, byteAA = {0}, charAA = {' '}, doubleAA = {Double.NaN, Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY},
            booleanAA = {true}, longAA = {Long.MAX_VALUE}, floatAA = {Float.NaN, Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY}, shortAA = {0},
            stringA = "", stringAA = "", classA = Thread.class, classAA = Thread.class, stateA = Thread.State.BLOCKED,
            stateAA = Thread.State.NEW, annoBA = @AnnoB, annoBAA = {@AnnoB, @AnnoB})
    Test(Object... aaa){
    }
}
// DEPENDENCE: AnnoA.java AnnoB.java
// EXEC:%maple  %f AnnoA.java AnnoB.java %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
