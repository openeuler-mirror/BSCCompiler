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
public class AnnotationNotAscii {
    static int[] passCnt = new int[24];
    public static void main(String[] args) {
        AnnoA anno = Test.class.getAnnotation(AnnoA.class);
        // primitive type
        int i = 0;
        passCnt[i++] = anno.整型() == Integer.MAX_VALUE ? 1: 0;
        passCnt[i++] += anno.字节() == Byte.MAX_VALUE ? 1: 0;
        passCnt[i++] += anno.字符() == Character.MAX_VALUE ? 1: 0;
        passCnt[i++] += Double.isNaN(anno.双精度浮点()) ? 1: 0;
        passCnt[i++] += anno.布尔() ? 1: 0;
        passCnt[i++] += anno.长整() == Long.MAX_VALUE ? 1: 0;
        passCnt[i++] += Float.isNaN(anno.浮点())? 1: 0;
        passCnt[i++] += anno.短整型() == Short.MAX_VALUE ? 1: 0;
        // enum, string, annotation, class
        passCnt[i++] += anno.类类类() == Thread.State.BLOCKED ? 1: 0;
        passCnt[i++] += anno.长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长().compareTo("") == 0 ? 1: 0;
        passCnt[i++] += anno.类() == Thread.class ? 1: 0;
        passCnt[i++] += Arrays.toString(anno.あ()).compareTo("[1, 2]") == 0 ? 1: 0;
        // array
        passCnt[i++] += (anno.い().length == 1 && anno.い()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.う().length == 1 && anno.う()[0] == ' ') ? 1: 0;
        passCnt[i++] += (anno.え().length == 3 && Double.isNaN(anno.え()[0]) && Double.isInfinite(anno.え()[1]) && Double.isInfinite(anno.え()[2]))? 1: 0;
        passCnt[i++] += (anno.お().length == 1 && anno.お()[0]) ? 1: 0;
        passCnt[i++] += (anno.か().length == 1 && anno.か()[0] == Long.MAX_VALUE) ? 1: 0;
        passCnt[i++] += (anno.き().length == 3 && Float.isNaN(anno.き()[0]) && Float.isInfinite(anno.き()[1]) && Float.isInfinite(anno.き()[2])) ? 1: 0;
        passCnt[i++] += (anno.く().length == 1 && anno.く()[0] == 0) ? 1: 0;
        passCnt[i++] += (anno.神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马().length == 1 && anno.神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马()[0].compareTo("")==0) ? 1: 0;
        passCnt[i++] += (anno.类类().length == 1 && anno.类类()[0] == Thread.class)? 1: 0;
        passCnt[i++] += (anno.类类类类().length == 1 && anno.类类类类()[0] == Thread.State.NEW) ? 1: 0;
        passCnt[i++] += anno.类类类类类().toString().compareTo("@AnnoB(intB=999)")==0 ? 1: 0;
        passCnt[i++] += Arrays.toString(anno.类类类类类类()).compareTo("[@AnnoB(intB=999), @AnnoB(intB=999)]") == 0 ? 1: 0;
        System.out.println(Arrays.toString(passCnt).compareTo("[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]"));
    }
}
@AnnoA(整型 = Integer.MAX_VALUE, 字节 = Byte.MAX_VALUE, 字符 = Character.MAX_VALUE, 双精度浮点 = Double.NaN,
        布尔 = true, 长整 = Long.MAX_VALUE, 浮点 = Float.NaN, 短整型 = Short.MAX_VALUE,
        あ = {1,2}, い = {0}, う = {' '}, え = {Double.NaN, Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY},
        お = {true}, か = {Long.MAX_VALUE}, き = {Float.NaN, Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY}, く = {0},
        长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长 = "", 神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马 = "", 类 = Thread.class, 类类 = Thread.class, 类类类 = Thread.State.BLOCKED,
        类类类类 = Thread.State.NEW, 类类类类类 = @AnnoB, 类类类类类类 = {@AnnoB, @AnnoB})
class Test{
}
