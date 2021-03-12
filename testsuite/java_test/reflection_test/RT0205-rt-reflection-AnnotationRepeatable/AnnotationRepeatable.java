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
public class AnnotationRepeatable {
    static int[] passCnt = new int[25];
    public static void main(String[] args) {
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[0], "[@AnnoB(intB=-1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[1], "[@AnnoB(intB=-2), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[2], "[@AnnoB(intB=-3), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[3], "[@AnnoB(intB=-4), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[4], "[@AnnoB(intB=-5), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[5], "[@AnnoB(intB=-6), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[6], "[@AnnoB(intB=-7), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[7], "[@AnnoB(intB=-8), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[8], "[@AnnoB(intB=-9), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        checkAnnoA(Test.class.getAnnotation(Test.class).value()[9], "[@AnnoB(intB=-10), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1), @AnnoB(intB=1)]");
        boolean checkAnnoA = Arrays.toString(passCnt).compareTo("[1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]") == 0;
        boolean checkAnnoB = Arrays.toString(AnnoA.class.getAnnotation(AnnoA.class).value()).compareTo("[@AnnoB(intB=1), @AnnoB(intB=2), @AnnoB(intB=3), @AnnoB(intB=4), @AnnoB(intB=5), @AnnoB(intB=6), @AnnoB(intB=7), @AnnoB(intB=8), @AnnoB(intB=9), @AnnoB(intB=10), @AnnoB(intB=11), @AnnoB(intB=12), @AnnoB(intB=13), @AnnoB(intB=14), @AnnoB(intB=15), @AnnoB(intB=16), @AnnoB(intB=17), @AnnoB(intB=18)]") == 0;
        if (checkAnnoA && checkAnnoB){
            System.out.println(0);
        } else {
            System.out.println(2);
        }
    }
    private static void checkAnnoA(AnnoA anno, String value){
        int i = 0;
        passCnt[i++] = anno.intA() == 0 ? 1: 0;
        passCnt[i++] += anno.byteA() == 0 ? 1: 0;
        passCnt[i++] += anno.charA() == 0 ? 1: 0;
        passCnt[i++] += anno.doubleA() == 0 ? 1: 0;
        passCnt[i++] += anno.booleanA() ? 1: 0;
        passCnt[i++] += anno.longA() == 0 ? 1: 0;
        passCnt[i++] += anno.floatA() == 0? 1: 0;
        passCnt[i++] += anno.shortA() == 0 ? 1: 0;
        // enum, string, annotation, class
        passCnt[i++] += anno.stateA() == Thread.State.BLOCKED ? 1: 0;
        passCnt[i++] += anno.stringA().compareTo("") == 0 ? 1: 0;
        passCnt[i++] += anno.classA() == Thread.class ? 1: 0;
        passCnt[i++] += anno.annoBA().toString().compareTo("@AnnoB(intB=999)")==0 ? 1: 0;
        // array
        passCnt[i++] += Arrays.toString(anno.intAA()).compareTo("[0]") == 0 ? 1: 0;
        passCnt[i++] += (anno.byteAA().length == 1 && anno.byteAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.charAA().length == 1 && anno.charAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.doubleAA().length == 1 && anno.doubleAA()[0] == 0)? 1: 0;
        passCnt[i++] += (anno.booleanAA().length == 1 && anno.booleanAA()[0]) ? 1: 0;
        passCnt[i++] += (anno.longAA().length == 1 && anno.longAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.floatAA().length == 1 && anno.floatAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.shortAA().length == 1 && anno.shortAA()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.stringAA().length == 1 && anno.stringAA()[0].compareTo("")==0) ? 1: 0;
        passCnt[i++] += (anno.classAA().length == 1 && anno.classAA()[0] == Thread.class)? 1: 0;
        passCnt[i++] += (anno.stateAA().length == 1 && anno.stateAA()[0] == Thread.State.BLOCKED) ? 1: 0;
        passCnt[i++] += Arrays.toString(anno.annoBAA()).compareTo("[@AnnoB(intB=999)]") == 0 ? 1: 0;
        // value
        passCnt[i++] += Arrays.toString(anno.value()).compareTo(value) == 0 ? 1: 0;
    }
}
