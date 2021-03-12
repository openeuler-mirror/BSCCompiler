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


import java.io.PrintStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
public class StringChineseClassTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2/*STATUS_FAILED*/;
        try {
            result = StringChineseClassTest_1();
        } catch (Exception e) {
            System.out.println(e);
            processResult -= 10;
        }
//        System.out.println(result);
//        System.out.println(randomtest.res);
        if (result == 1 && processResult == 95) {
            result = 0;
        }
        return result;
    }
    public static int StringChineseClassTest_1() {
        int result1 = 4; /*STATUS_FAILED*/
        try {
            中国 test1 = new 中国();
            Method m1 = 中国.class.getDeclaredMethod("stringtestmethod");
            m1.invoke(test1);
            あなたのことが好きです test2 = new あなたのことが好きです();
            Method m2 = あなたのことが好きです.class.getDeclaredMethod("stringtestmethod");
            m2.invoke(test2);
            사랑해 test3 = new 사랑해();
            Method m3 = 사랑해.class.getDeclaredMethod("stringtestmethod");
            m3.invoke(test3);
            processResult -= 4;
        } catch (NoSuchMethodException e) {
            System.out.println(e);
        } catch (IllegalAccessException e) {
            System.out.println(e);
        } catch (InvocationTargetException e) {
            System.out.println(e);
        }
        return 1;///*STATUS_PASSED*/
    }
}
class 中国 {
    public void stringtestmethod() {
        System.out.println("hello，中文，中国");
    }
}
class あなたのことが好きです {
    public void stringtestmethod() {
        System.out.println("hello，日语，あなたのことが好きです");
    }
}
class 사랑해 {
    public void stringtestmethod() {
        System.out.println("hello，韩语，사랑해");
    }
}
