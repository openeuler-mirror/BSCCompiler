package com.huawei;

import java.util.Arrays;

public class AnnotationPackageSetTest {
    static int[] passCnt;
    public static void main(String[] args) {
        Package packageInfo = Package.getPackage("com.huawei");
        AnnoA anno = packageInfo.getDeclaredAnnotation(AnnoA.class);
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
        passCnt[i++] += anno.annoBA().toString().compareTo("@com.huawei.AnnoB(intB=999)")==0 ? 1: 0;
        passCnt[i++] += Arrays.toString(anno.annoBAA()).compareTo("[@com.huawei.AnnoB(intB=999), @com.huawei.AnnoB(intB=999)]") == 0 ? 1: 0;
        System.out.println(Arrays.toString(passCnt).compareTo("[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]"));
    }
}
// DEPENDENCE: AnnoA.java AnnoB.java package-info.java
// EXEC:%maple  %f AnnoA.java AnnoB.java package-info.java %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
