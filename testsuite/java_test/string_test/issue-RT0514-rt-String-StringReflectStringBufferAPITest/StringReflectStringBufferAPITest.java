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
public class StringReflectStringBufferAPITest {
//    private static int processResult = 599;
    public static void main(String[] args) {
        System.out.println(run(args, System.out));
    }
    public static int run(String[] args, PrintStream out) {
        int result = 2;
        try {
            StringReflectStringBufferAPITest1();
            result = 0;
        } catch (Exception e) {
        }
        return result;
    }
    public static void StringReflectStringBufferAPITest1() {
        StringBuffer sb1 = new StringBuffer();
        StringBuffer sb2 = new StringBuffer("abc123");
        StringBuffer sb3 = new StringBuffer("      ");
        StringBuffer sb4 = new StringBuffer("");
        StringBuffer sb5 = new StringBuffer("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
        System.out.println("==========test1==========");
        test1(sb1);
        test1(sb2);
        test1(sb3);
        test1(sb4);
        test1(sb5);
        System.out.println("==========test2==========");
        test2(sb1);
        test2(sb2);
        test2(sb3);
        test2(sb4);
        test2(sb5);
        System.out.println("==========test3==========");
        test3(sb1);
        test3(sb2);
        test3(sb3);
        test3(sb4);
        test3(sb5);
        System.out.println("==========test4==========");
        test4(sb1);
        test4(sb2);
        test4(sb3);
        test4(sb4);
        test4(sb5);
        System.out.println("==========test5==========");
        test5(sb1);
        test5(sb2);
        test5(sb3);
        test5(sb4);
        test5(sb5);
        System.out.println("==========test6==========");
        test6(sb1);
        test6(sb2);
        test6(sb3);
        test6(sb4);
        test6(sb5);
        System.out.println("==========test7==========");
        test7(sb1);
        test7(sb2);
        test7(sb3);
        test7(sb4);
        test7(sb5);
        System.out.println("==========test8==========");
        test8(sb1);
        test8(sb2);
        test8(sb3);
        test8(sb4);
        test8(sb5);
        System.out.println("==========test9==========");
        test9(sb1);
        test9(sb2);
        test9(sb3);
        test9(sb4);
        test9(sb5);
        System.out.println("==========test10==========");
        test10(sb1);
        test10(sb2);
        test10(sb3);
        test10(sb4);
        test10(sb5);
        System.out.println("==========test11==========");
        test11(sb1);
        test11(sb2);
        test11(sb3);
        test11(sb4);
        test11(sb5);
        System.out.println("==========test12==========");
        test12(sb1);
        test12(sb2);
        test12(sb3);
        test12(sb4);
        test12(sb5);
        System.out.println("==========test13==========");
        test13(sb1);
        test13(sb2);
        test13(sb3);
        test13(sb4);
        test13(sb5);
        System.out.println("==========test14==========");
        test14(sb1);
        test14(sb2);
        test14(sb3);
        test14(sb4);
        test14(sb5);
        System.out.println("==========test15==========");
        test15(sb1);
        test15(sb2);
        test15(sb3);
        test15(sb4);
        test15(sb5);
        System.out.println("==========test16==========");
        test16(sb1);
        test16(sb2);
        test16(sb3);
        test16(sb4);
        test16(sb5);
        System.out.println("==========test17==========");
        test17(sb1);
        test17(sb2);
        test17(sb3);
        test17(sb4);
        test17(sb5);
        System.out.println("==========test18==========");
        test18(sb1);
        test18(sb2);
        test18(sb3);
        test18(sb4);
        test18(sb5);
        System.out.println("==========test19==========");
        test19(sb1);
        test19(sb2);
        test19(sb3);
        test19(sb4);
        test19(sb5);
        System.out.println("==========test20==========");
        test20(sb1);
        test20(sb2);
        test20(sb3);
        test20(sb4);
        test20(sb5);
        System.out.println("==========test21==========");
        test21(sb1);
        test21(sb2);
        test21(sb3);
        test21(sb4);
        test21(sb5);
        System.out.println("==========test22==========");
        test22(sb1);
        test22(sb2);
        test22(sb3);
        test22(sb4);
        test22(sb5);
        System.out.println("==========test23==========");
        test23(sb1);
        test23(sb2);
        test23(sb3);
        test23(sb4);
        test23(sb5);
        System.out.println("==========test24==========");
        test24(sb1);
        test24(sb2);
        test24(sb3);
        test24(sb4);
        test24(sb5);
        System.out.println("==========test25==========");
        test25(sb1);
        test25(sb2);
        test25(sb3);
        test25(sb4);
        test25(sb5);
        System.out.println("==========test26==========");
        test26(sb1);
        test26(sb2);
        test26(sb3);
        test26(sb4);
        test26(sb5);
        System.out.println("==========test27==========");
        test27(sb1);
        test27(sb2);
        test27(sb3);
        test27(sb4);
        test27(sb5);
        System.out.println("==========test28==========");
        test28(sb1);
        test28(sb2);
        test28(sb3);
        test28(sb4);
        test28(sb5);
        System.out.println("==========test29==========");
        test29(sb1);
        test29(sb2);
        test29(sb3);
        test29(sb4);
        test29(sb5);
        System.out.println("==========test30==========");
        test30(sb1);
        test30(sb2);
        test30(sb3);
        test30(sb4);
        test30(sb5);
        System.out.println("==========test31==========");
        test31(sb1);
        test31(sb2);
        test31(sb3);
        test31(sb4);
        test31(sb5);
        System.out.println("==========test32==========");
        test32(sb1);
        test32(sb2);
        test32(sb3);
        test32(sb4);
        test32(sb5);
        System.out.println("==========test33==========");
        test33(sb1);
        test33(sb2);
        test33(sb3);
        test33(sb4);
        test33(sb5);
        System.out.println("==========test34==========");
        test34(sb1);
        test34(sb2);
        test34(sb3);
        test34(sb4);
        test34(sb5);
        System.out.println("==========test35==========");
        test35(sb1);
        test35(sb2);
        test35(sb3);
        test35(sb4);
        test35(sb5);
        System.out.println("==========test36==========");
        test36(sb1);
        test36(sb2);
        test36(sb3);
        test36(sb4);
        test36(sb5);
        System.out.println("==========test37==========");
        test37(sb1);
        test37(sb2);
        test37(sb3);
        test37(sb4);
        test37(sb5);
        System.out.println("==========test38==========");
        test38(sb1);
        test38(sb2);
        test38(sb3);
        test38(sb4);
        test38(sb5);
        System.out.println("==========test39==========");
        test39(sb1);
        test39(sb2);
        test39(sb3);
        test39(sb4);
        test39(sb5);
        System.out.println("==========test40==========");
        test40(sb1);
        test40(sb2);
        test40(sb3);
        test40(sb4);
        test40(sb5);
        System.out.println("==========test41==========");
        test41(sb1);
        test41(sb2);
        test41(sb3);
        test41(sb4);
        test41(sb5);
        System.out.println("==========test42==========");
        test42(sb1);
        test42(sb2);
        test42(sb3);
        test42(sb4);
        test42(sb5);
        System.out.println("==========test43==========");
        test43(sb1);
        test43(sb2);
        test43(sb3);
        test43(sb4);
        test43(sb5);
        System.out.println("==========test44==========");
        test44(sb1);
        test44(sb2);
        test44(sb3);
        test44(sb4);
        test44(sb5);
        System.out.println("==========test45==========");
        test45(sb1);
        test45(sb2);
        test45(sb3);
        test45(sb4);
        test45(sb5);
        System.out.println("==========test46==========");
        test46(sb1);
        test46(sb2);
        test46(sb3);
        test46(sb4);
        test46(sb5);
        System.out.println("==========test47==========");
        test47(sb1);
        test47(sb2);
        test47(sb3);
        test47(sb4);
        test47(sb5);
        System.out.println("==========test48==========");
        test48(sb1);
        test48(sb2);
        test48(sb3);
        test48(sb4);
        test48(sb5);
        System.out.println("==========test49==========");
        test49(sb1);
        test49(sb2);
        test49(sb3);
        test49(sb4);
        test49(sb5);
        System.out.println("==========test50==========");
        test50(sb1);
        test50(sb2);
        test50(sb3);
        test50(sb4);
        test50(sb5);
    }
    public static void test1(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {true};
        try {
            Method method = class1.getMethod("append", new Class[]{boolean.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test2(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {999999999};
        try {
            Method method = class1.getMethod("append", new Class[]{long.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test3(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {'I'};
        try {
            Method method = class1.getMethod("append", new Class[]{char.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test4(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {"I like Reading"};
        try {
            Method method = class1.getMethod("append", new Class[]{Object.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test5(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        char[] chars = {'a', 'b', 'c'};
        Object[] objects = {chars, 0, 1};
        try {
            Method method = class1.getMethod("append", char[].class, int.class, int.class);
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test6(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {20.56};
        try {
            Method method = class1.getMethod("append", new Class[]{double.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test7(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        char[] chars = {'a', 'A'};
        Object[] objects = {chars};
        try {
            Method method = class1.getMethod("append", new Class[]{char[].class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test8(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {"abcdefg"};
        try {
            Method method = class1.getMethod("append", new Class[]{String.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test9(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        StringBuffer str = new StringBuffer("str");
        Object[] objects = {str};
        try {
            Method method = class1.getMethod("append", new Class[]{StringBuffer.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test10(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        float f = (float) 10.0;
        Object[] objects = {f};
        try {
            Method method = class1.getMethod("append", new Class[]{float.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test11(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {100};
        try {
            Method method = class1.getMethod("append", new Class[]{int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test12(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        CharSequence charSequence = new StringBuffer("LLL");
        Object[] objects = {charSequence};
        try {
            Method method = class1.getMethod("append", new Class[]{CharSequence.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test13(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        int unicode = 0x5b57;// 追加unicode编码
        Object[] objects = {unicode};
        try {
            Method method = class1.getMethod("appendCodePoint", new Class[]{int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test14(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        //Object[] objects = {};
        try {
            Method method = class1.getMethod("capacity");
            int sb = (int) method.invoke(stringBuffer);
            System.out.println(sb);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test15(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {5};
        try {
            Method method = class1.getMethod("charAt", new Class[]{int.class});
            char sb = (char) method.invoke(stringBuffer, objects);
            System.out.println(sb);
//            if ("9".equals(String.valueOf(sb))||"3".equals(String.valueOf(sb))||
//                    " ".equals(String.valueOf(sb))||"9".equals(String.valueOf(sb))||"&".equals(String.valueOf(sb))){
//
//            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test16(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {5};
        try {
            Method method = class1.getMethod("codePointAt", new Class[]{int.class});
            int sb = (int) method.invoke(stringBuffer, objects);
            System.out.println(sb);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test17(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {5};
        try {
            Method method = class1.getMethod("codePointBefore", new Class[]{int.class});
            int sb = (int) method.invoke(stringBuffer, objects);
            System.out.println(sb);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test18(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {5, 8};
        try {
            Method method = class1.getMethod("codePointCount", new Class[]{int.class, int.class});
            int sb = (int) method.invoke(stringBuffer, objects);
            System.out.println(sb);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test19(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {1, 3};
        try {
            Method method = class1.getMethod("delete", new Class[]{int.class, int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test20(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {5};
        try {
            Method method = class1.getMethod("deleteCharAt", new Class[]{int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test21(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {50};
        try {
            Method method = class1.getMethod("ensureCapacity", new Class[]{int.class});
            method.invoke(stringBuffer, objects);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test22(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        char[] chars = {'R', 'T', 'Y', 'U', 'P', 'W', 'X', 'F', 'J'};
        Object[] objects = {2, 5, chars, 2};
        try {
            Method method = class1.getMethod("getChars", new Class[]{int.class, int.class, char[].class, int.class});
            method.invoke(stringBuffer, objects);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test23(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {"9"};
        try {
            Method method = class1.getMethod("indexOf", new Class[]{String.class});
            int index = (int) method.invoke(stringBuffer, objects);
            System.out.println(index);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test24(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {"9", 1};
        try {
            Method method = class1.getMethod("indexOf", new Class[]{String.class, int.class});//从int位置往后开始数，该字符第一次出现的位置
            int index = (int) method.invoke(stringBuffer, objects);
            System.out.println(index);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test25(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        char[] chars = {'H', 'J', 'K', 'S'};
        Object[] objects = {5, chars};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, char[].class});//从int位置往后开始数，该字符第一次出现的位置
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test26(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        float f = (float) 1.0;
        Object[] objects = {5, f};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, float.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test27(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        CharSequence sequence = new StringBuffer("######");
        Object[] objects = {5, sequence};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, CharSequence.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test28(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        char chars = '@';
        Object[] objects = {5, chars};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, char.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test29(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {10, 888888888};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, long.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test30(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        char[] chars = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
        Object[] objects = {15, chars, 0, 3};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, char[].class, int.class, int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test31(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {5, 33333};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test32(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, "UUUUU"};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, String.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test33(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, 100.00};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, double.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test34(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        CharSequence sequence = new StringBuffer("YYYYYYYYYYY");
        Object[] objects = {0, sequence, 0, 5};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, CharSequence.class, int.class, int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test35(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, 77777};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, Object.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test36(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, false};
        try {
            Method method = class1.getMethod("insert", new Class[]{int.class, boolean.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test37(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {"U", 50};
        try {
            Method method = class1.getMethod("lastIndexOf", new Class[]{String.class, int.class});//从指定的下标往前找，找指定字符串最后一次出现的位置
            int index = (int) method.invoke(stringBuffer, objects);
            System.out.println(index);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test38(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {"Y"};
        try {
            Method method = class1.getMethod("lastIndexOf", new Class[]{String.class});
            int index = (int) method.invoke(stringBuffer, objects);
            System.out.println(index);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test39(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {};
        try {
            Method method = class1.getMethod("length");
            int index = (int) method.invoke(stringBuffer, objects);
            System.out.println(index);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test40(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, 3};
        try {
            Method method = class1.getMethod("offsetByCodePoints", new Class[]{int.class, int.class});//返回此 String 中从给定的 index 处偏移 codePointOffset 个代码点的索引。
            int index = (int) method.invoke(stringBuffer, objects);
            System.out.println(index);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test41(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, 5, "PPPPP"};
        try {
            Method method = class1.getMethod("replace", new Class[]{int.class, int.class, String.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test42(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        //Object[] objects = {};
        try {
            Method method = class1.getMethod("reverse");
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test43(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, '#'};
        try {
            Method method = class1.getMethod("setCharAt", new Class[]{int.class, char.class});
            method.invoke(stringBuffer, objects);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test44(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {210};
        try {
            Method method = class1.getMethod("setLength", new Class[]{int.class});
            method.invoke(stringBuffer, objects);
            String var = stringBuffer.toString();
            System.out.println(var.length());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test45(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, 5};
        try {
            Method method = class1.getMethod("subSequence", new Class[]{int.class, int.class});
            CharSequence charSequence = (CharSequence) method.invoke(stringBuffer, objects);
            System.out.println(charSequence);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test46(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        CharSequence charSequence = new StringBuffer("WWWWWWWWW");
        Object[] objects = {charSequence, 0, 3};
        try {
            Method method = class1.getMethod("append", new Class[]{CharSequence.class, int.class, int.class});
            StringBuffer sb = (StringBuffer) method.invoke(stringBuffer, objects);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test47(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {0, 5};
        try {
            Method method = class1.getMethod("substring", new Class[]{int.class, int.class});
            String string = (String) method.invoke(stringBuffer, objects);
            System.out.println(string);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test48(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        Object[] objects = {210};
        try {
            Method method = class1.getMethod("substring", new Class[]{int.class});
            String string = (String) method.invoke(stringBuffer, objects);
            System.out.println(string);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test49(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        //Object[] objects = {};
        try {
            Method method = class1.getMethod("toString");
            String string = (String) method.invoke(stringBuffer);
            //System.out.println(string);
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
    public static void test50(StringBuffer stringBuffer) {
        Class class1 = stringBuffer.getClass();
        //Object[] objects = {};
        try {
            Method method = class1.getMethod("trimToSize");//该方法的作用是将StringBuffer对象的中存储空间缩小到和字符串长度一样的长度，减少空间的浪费。
            method.invoke(stringBuffer);
            System.out.println(stringBuffer.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
    }
}
