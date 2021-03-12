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
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
public class StringReflectAPITest {
    private static int processResult = 700;
    private static String Charset;
    public static void main(String argv[]) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2/*STATUS_FAILED*/;
        try {
            StringReflectAPITest_1();
        } catch (Exception e) {
            e.printStackTrace();
            processResult -= 10;
        }
        //System.out.println(result);
        //System.out.println(StringReflectAPITest.res);
        if (result == 2 && processResult == 126) {
            result = 0;
        }
        return result;
    }
    private static void StringReflectAPITest_1() throws SecurityException, IllegalArgumentException {
        int result1 = 4;/*STATUS_FAILED*/
        String s1 = null;
        String s2 = "abc123";
        String s3 = "      ";
        String s4 = "";
        String s5 = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        test1(s1);
        test1(s2);
        test1(s3);
        test1(s4);
        test1(s5);
        test2(s1);
        test2(s2);
        test2(s3);
        test2(s4);
        test2(s5);
        test3(s1);
        test3(s2);
        test3(s3);
        test3(s4);
        test3(s5);
        test4(s1);
        test4(s2);
        test4(s3);
        test4(s4);
        test4(s5);
        test5(s1);
        test5(s2);
        test5(s3);
        test5(s4);
        test5(s5);
        test6(s1);
        test6(s2);
        test6(s3);
        test6(s4);
        test6(s5);
        test7(s1);
        test7(s2);
        test7(s3);
        test7(s4);
        test7(s5);
        test8(s1);
        test8(s2);
        test8(s3);
        test8(s4);
        test8(s5);
        test9(s1);
        test9(s2);
        test9(s3);
        test9(s4);
        test9(s5);
        test10(s1);
        test10(s2);
        test10(s3);
        test10(s4);
        test10(s5);
        test11(s1);
        test11(s2);
        test11(s3);
        test11(s4);
        test11(s5);
        test12(s1);
        test12(s2);
        test12(s3);
        test12(s4);
        test12(s5);
        test13(s1);
        test13(s2);
        test13(s3);
        test13(s4);
        test13(s5);
        test14(s1);
        test14(s2);
        test14(s3);
        test14(s4);
        test14(s5);
        test15(s1);
        test15(s2);
        test15(s3);
        test15(s4);
        test15(s5);
        test16(s1);
        test16(s2);
        test16(s3);
        test16(s4);
        test16(s5);
        test17(s1);
        test17(s2);
        test17(s3);
        test17(s4);
        test17(s5);
        test18(s1);
        test18(s2);
        test18(s3);
        test18(s4);
        test18(s5);
        test19(s1);
        test19(s2);
        test19(s3);
        test19(s4);
        test19(s5);
		/*test20(s1);
		test20(s2);
		test20(s3);
		test20(s4);
		test20(s5);*/
        test21(s1);
        test21(s2);
        test21(s3);
        test21(s4);
        test21(s5);
        test22(s1);
        test22(s2);
        test22(s3);
        test22(s4);
        test22(s5);
        test23(s1);
        test23(s2);
        test23(s3);
        test23(s4);
        test23(s5);
        test24(s1);
        test24(s2);
        test24(s3);
        test24(s4);
        test24(s5);
        test25(s1);
        test25(s2);
        test25(s3);
        test25(s4);
        test25(s5);
        test26(s1);
        test26(s2);
        test26(s3);
        test26(s4);
        test26(s5);
        test28(s1);
        test28(s2);
        test28(s3);
        test28(s4);
        test28(s5);
        test29(s1);
        test29(s2);
        test29(s3);
        test29(s4);
        test29(s5);
        test30(s1);
        test30(s2);
        test30(s3);
        test30(s4);
        test30(s5);
        test31(s1);
        test31(s2);
        test31(s3);
        test31(s4);
        test31(s5);
        test32(s1);
        test32(s2);
        test32(s3);
        test32(s4);
        test32(s5);
        test33(s1);
        test33(s2);
        test33(s3);
        test33(s4);
        test33(s5);
        test34(s1);
        test34(s2);
        test34(s3);
        test34(s4);
        test34(s5);
        test35(s1);
        test35(s2);
        test35(s3);
        test35(s4);
        test35(s5);
        test36(s1);
        test36(s2);
        test36(s3);
        test36(s4);
        test36(s5);
        test37(s1);
        test37(s2);
        test37(s3);
        test37(s4);
        test37(s5);
        test38(s1);
        test38(s2);
        test38(s3);
        test38(s4);
        test38(s5);
        test39(s1);
        test39(s2);
        test39(s3);
        test39(s4);
        test39(s5);
        test40(s1);
        test40(s2);
        test40(s3);
        test40(s4);
        test40(s5);
        test41(s1);
        test41(s2);
        test41(s3);
        test41(s4);
        test41(s5);
        test42(s1);
        test42(s2);
        test42(s3);
        test42(s4);
        test42(s5);
        test43(s1);
        test43(s2);
        test43(s3);
        test43(s4);
        test43(s5);
        test44(s1);
        test44(s2);
        test44(s3);
        test44(s4);
        test44(s5);
        test45(s1);
        test45(s2);
        test45(s3);
        test45(s4);
        test45(s5);
        test46(s1);
        test46(s2);
        test46(s3);
        test46(s4);
        test46(s5);
        test47(s1);
        test47(s2);
        test47(s3);
        test47(s4);
        test47(s5);
        test48(s1);
        test48(s2);
        test48(s3);
        test48(s4);
        test48(s5);
        test49(s1);
        test49(s2);
        test49(s3);
        test49(s4);
        test49(s5);
        test50(s1);
        test50(s2);
        test50(s3);
        test50(s4);
        test50(s5);
        test51(s1);
        test51(s2);
        test51(s3);
        test51(s4);
        test51(s5);
        test52(s1);
        test52(s2);
        test52(s3);
        test52(s4);
        test52(s5);
        test53(s1);
        test53(s2);
        test53(s3);
        test53(s4);
        test53(s5);
        test54(s1);
        test54(s2);
        test54(s3);
        test54(s4);
        test54(s5);
        test55(s1);
        test55(s2);
        test55(s3);
        test55(s4);
        test55(s5);
        test56(s1);
        test56(s2);
        test56(s3);
        test56(s4);
        test56(s5);
        test57(s1);
        test57(s2);
        test57(s3);
        test57(s4);
        test57(s5);
        test58(s1);
        test58(s2);
        test58(s3);
        test58(s4);
        test58(s5);
        test59(s1);
        test59(s2);
        test59(s3);
        test59(s4);
        test59(s5);
        test60(s1);
        test60(s2);
        test60(s3);
        test60(s4);
        test60(s5);
        test61(s1);
        test61(s2);
        test61(s3);
        test61(s4);
        test61(s5);
        test62(s1);
        test62(s2);
        test62(s3);
        test62(s4);
        test62(s5);
        test63(s1);
        test63(s2);
        test63(s3);
        test63(s4);
        test63(s5);
        test64(s1);
        test64(s2);
        test64(s3);
        test64(s4);
        test64(s5);
        test65(s1);
        test65(s2);
        test65(s3);
        test65(s4);
        test65(s5);
        test66(s1);
        test66(s2);
        test66(s3);
        test66(s4);
        test66(s5);
        test67(s1);
        test67(s2);
        test67(s3);
        test67(s4);
        test67(s5);
        test68(s1);
        test68(s2);
        test68(s3);
        test68(s4);
        test68(s5);
    }
    private static void test1(String string) {
        //Class class1 = string.getClass();
        //Object[] objects = {"a","b","c"};
        if (string != null) {
            try {
                Class argsClass[] = {int.class};
                Object args[] = {1};
                Method m = String.class.getMethod("charAt", argsClass);
                char result = (char) m.invoke(string, args);
                if (String.valueOf(result) != null) {
                    processResult -= 2;
                }
            } catch (IndexOutOfBoundsException e) {
                //e.printStackTrace();
                processResult -= 2;
            } catch (NoSuchMethodException e) {
                e.printStackTrace();
            } catch (SecurityException e) {
                e.printStackTrace();
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
            } catch (InvocationTargetException e) {
                //e.printStackTrace();
                processResult -= 2;
            }
        } else {
            try {
                char chars = string.charAt(2);
            } catch (NullPointerException e) {
                //e.printStackTrace();
                processResult -= 2;
            }
        }
    }
    private static void test2(String string) {
        try {
            Class argsClass[] = {int.class};
            Object args[] = {1};
            Method m = String.class.getMethod("codePointAt", argsClass);//codePointAt(int index)
            int result = (int) m.invoke(string, args);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            //System.out.println("===============fail2");
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test3(String string) {
        try {
            Class argsClass[] = {int.class};
            Object args[] = {1};
            Method m = String.class.getMethod("codePointBefore", argsClass);//codePointBefore(int index)
            int result = (int) m.invoke(string, args);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test4(String string) {
        try {
            Class argsClass[] = {int.class, int.class};
            Object args[] = {1, 3};
            Method m = String.class.getMethod("codePointCount", new Class[]{int.class, int.class});//	codePointCount(int beginIndex, int endIndex)
            int result = (int) m.invoke(string, args);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        } catch (IndexOutOfBoundsException e) {
            //e.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test5(String string) {
        try {
            Class params[] = {String.class};
            Class argsClass[] = {String.class};
            Object args[] = {"adc"};
            Method m = String.class.getMethod("compareTo", argsClass);//compareTo(String anotherString)
            int result = (int) m.invoke(string, args);
            if (result < 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test6(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"adc"};
            Method m = String.class.getMethod("compareToIgnoreCase", argsClass);//compareToIgnoreCase(String str)
            int result = (int) m.invoke(string, args);
            if (result < 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test7(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"QWE"};
            Method m = String.class.getMethod("concat", argsClass);//concat(String str)
            Object result = m.invoke(string, args);
            if (result.toString().contains("QWE")) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test8(String string) {
        CharSequence charSequence = new String("abc");
        try {
            Class argsClass[] = {CharSequence.class};
            Object args[] = {charSequence};
            Method m = String.class.getMethod("contains", argsClass);//contains(CharSequence s)
            boolean result = (boolean) m.invoke(string, args);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test9(String string) {
        try {
            Class[] argsClass = {StringBuffer.class};
            Object args[] = {new StringBuffer("abc123")};
            Method m = String.class.getMethod("contentEquals", argsClass);//contentEquals(StringBuffer sb)
            boolean result = (boolean) m.invoke(string, args);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test10(String string) {
        CharSequence charSequence = new String("abc123");
        try {
            Class argsClass[] = {CharSequence.class};
            Object args[] = {charSequence};
            Method m = String.class.getMethod("contentEquals", argsClass);//contentEquals(CharSequence cs)
            boolean result = (boolean) m.invoke(string, args);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test11(String string) {
        char[] chars = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
        try {
            Class argsClass[] = {char[].class, int.class, int.class};
            Object args[] = {chars, 1, 5};
            Method m = String.class.getMethod("copyValueOf", argsClass);//	copyValueOf(char[] data, int offset, int count)
            String result = (String) m.invoke(string, args);
            if ("bcdef".equals(result.toString())) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
            //e.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test12(String string) {
        char[] chars = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
        try {
            Class[] argsClass = {char[].class};
            Object args[] = {chars};
            Method m = String.class.getMethod("copyValueOf", argsClass);//copyValueOf(char[] data)
            String result = (String) m.invoke(string, args);
            if ("abcdefg".equals(result.toString())) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test13(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"3"};
            Method m = String.class.getMethod("endsWith", argsClass);//endsWith(String suffix)
            boolean result = (boolean) m.invoke(string, args);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException e) {
            //e.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test14(String string) {
        try {
            Class argsClass[] = {Object.class};
            Object args[] = {"abc123"};
            Method m = String.class.getMethod("equals", argsClass);//equals(Object anObject)
            boolean result = (boolean) m.invoke(string, args);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test15(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"ABC123"};
            Method m = String.class.getMethod("equalsIgnoreCase", argsClass);//equalsIgnoreCase(String anotherString)
            boolean result = (boolean) m.invoke(string, args);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test16(String string) {
        try {
            Class[] argsClass = {String.class, Object[].class};
            Object[] objects = {99, "abc"};
            Object[] args = {"格式参数$的使用：%1$d,%2$s", objects};//LOCALE表示地区
            Method m = String.class.getMethod("format", argsClass);//format(Locale l, String format, Object... args)
            String result = (String) m.invoke(string, args);
            if ("格式参数$的使用：99,abc".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
            processResult -= 2;
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
    }
    private static void test17(String string) {
        try {
            Class argsClass[] = {String.class, Object[].class};
            Object[] objects = {"Tom"};
            Object args[] = {"Hi,%s", objects};
            Method m = String.class.getMethod("format", argsClass);//format(String format, Object... args)
            String result = (String) m.invoke(string, args);
            if ("Hi,Tom".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test18(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"ISO-8859-1"};
            Method m = String.class.getMethod("getBytes", argsClass);//getBytes(String charsetName)
            byte[] bytes = (byte[]) m.invoke(string, args);
            if (bytes.length >= 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test19(String string) {
        try {
            Method m = String.class.getMethod("getBytes");//getBytes()
            byte[] bytes = (byte[]) m.invoke(string);
            if (bytes.length >= 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
    }
    private static void test20(String string) {//有问题，待解决
        try {
            Class argsClass[] = {Charset.class};
            Object args[] = {"GBK"};
            Method m = String.class.getMethod("getBytes", argsClass);//getBytes(Charset charset)
            byte[] bytes = (byte[]) m.invoke(string, args);
            System.out.println(bytes.length);
            if (bytes.length >= 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        } catch (IllegalArgumentException e) {
            //e.printStackTrace();
        }
    }
    private static void test21(String string) {
        byte[] bytes = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        try {
            Class argsClass[] = {int.class, int.class, byte[].class, int.class};
            Object args[] = {1, 3, bytes, 2};
            Method m = String.class.getMethod("getBytes", argsClass);//getBytes(int srcBegin, int srcEnd, byte[] dst, int dstBegin)
            m.invoke(string, args);
            processResult -= 2;
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test22(String string) {
        try {
            Class argsClass[] = {int.class, int.class, char[].class, int.class};
            char[] chars = {'a', 'b', 'c', 'd', 'e'};
            Object args[] = {1, 3, chars, 2};
            Method m = String.class.getMethod("getChars", argsClass);//	getChars(int srcBegin, int srcEnd, char[] dst, int dstBegin)
            m.invoke(string, args);
            processResult -= 2;
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();	
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test23(String string) {
        try {
            Method m = String.class.getMethod("hashCode");//	hashCode()
            int result = (int) m.invoke(string);
            //System.out.println(result);
            if (String.valueOf(result) != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test24(String string) {
        try {
            Class argsClass[] = {int.class, int.class};
            Object args[] = {1, 3};
            Method m = String.class.getMethod("indexOf", argsClass);//	indexOf(int ch, int fromIndex)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (String.valueOf(result) != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test25(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"c"};
            Method m = String.class.getMethod("indexOf", argsClass);//indexOf(String str)
            int result = (int) m.invoke(string, args);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test26(String string) {
        try {
            Class argsClass[] = {int.class};
            Object args[] = {3};
            Method m = String.class.getMethod("indexOf", argsClass);//	indexOf(int ch)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result == -1) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test28(String string) {
        try {
            Class argsClass[] = {String.class, int.class};
            Object args[] = {"c", 0};
            Method m = String.class.getMethod("indexOf", argsClass);//indexOf(String str, int fromIndex)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test29(String string) {
        try {
            Method m = String.class.getMethod("intern");//intern()
            String result = (String) m.invoke(string);
            //System.out.println(result);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test30(String string) {
        try {
            Method m = String.class.getMethod("isEmpty");//isEmpty()
            boolean flag = (boolean) m.invoke(string);
            //System.out.println(flag);
            if (flag) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test31(String string) {
        try {
            Class argsClass[] = {CharSequence.class, CharSequence[].class};
            CharSequence[] charSequences = {"G"};
            Object args[] = {"F", charSequences};
            Method m = String.class.getMethod("join", argsClass);//join(CharSequence delimiter, CharSequence... elements)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if ("G".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test32(String string) {
        String delimiter = "-";
        String[] elements = {"JDK", "8", "String", "join"};
        List<String> list = new ArrayList<String>(Arrays.asList(elements));
        try {
            Class argsClass[] = {CharSequence.class, Iterable.class};
            Object args[] = {delimiter, list};
            Method m = String.class.getMethod("join", argsClass);//join(CharSequence delimiter, Iterable<? extends CharSequence> elements)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if ("JDK-8-String-join".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test33(String string) {
        try {
            Class argsClass[] = {int.class};
            Object args[] = {1};
            Method m = String.class.getMethod("lastIndexOf", argsClass);//	lastIndexOf(int ch)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result == -1) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test34(String string) {
        try {
            Class argsClass[] = {String.class, int.class};
            Object args[] = {"c", 0};
            Method m = String.class.getMethod("lastIndexOf", argsClass);//	lastIndexOf(String str, int fromIndex)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result == -1) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test35(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"c"};
            Method m = String.class.getMethod("lastIndexOf", argsClass);//lastIndexOf(String str)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test36(String string) {
        try {
            Class argsClass[] = {int.class, int.class};
            Object args[] = {1, 0};
            Method m = String.class.getMethod("lastIndexOf", argsClass);//	lastIndexOf(int ch, int fromIndex)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result == -1) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test37(String string) {
        try {
            Method m = String.class.getMethod("length");//length()
            int result = (int) m.invoke(string);
            if (result >= 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test38(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"abc123"};
            Method m = String.class.getMethod("matches", argsClass);//matches(String regex)
            boolean flag = (boolean) m.invoke(string, args);
            //System.out.println(flag);
            if (flag) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test39(String string) {
        try {
            Class argsClass[] = {int.class, int.class};
            Object args[] = {1, 2};
            Method m = String.class.getMethod("offsetByCodePoints", argsClass);//offsetByCodePoints(int index, int codePointOffset)
            int result = (int) m.invoke(string, args);
            //System.out.println(result);
            if (result > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test40(String string) {
        try {
            Class argsClass[] = {boolean.class, int.class, String.class, int.class, int.class};
            Object args[] = {true, 1, "abc123", 2, 3};
            Method m = String.class.getMethod("regionMatches", argsClass);//regionMatches(boolean ignoreCase, int toffset, String other, int ooffset, int len)
            boolean flag = (boolean) m.invoke(string, args);
            //System.out.println(flag);
            if (!flag) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test41(String string) {
        try {
            Class argsClass[] = {int.class, String.class, int.class, int.class};
            Object args[] = {0, "abc123", 1, 2};
            Method m = String.class.getMethod("regionMatches", argsClass);//	regionMatches(int toffset, String other, int ooffset, int len)
            boolean flag = (boolean) m.invoke(string, args);
            //System.out.println(flag);
            if (!flag) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test42(String string) {
        CharSequence charSequence = new String("abc");
        CharSequence charSequence1 = new String("ABC");
        try {
            Class argsClass[] = {CharSequence.class, CharSequence.class};
            Object args[] = {charSequence, charSequence1};
            Method m = String.class.getMethod("replace", argsClass);//	replace(charSequence  target, CharSequence replacement)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if (result.contains("ABC")) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test43(String string) {
        char ch = 'a';
        char ch1 = 'F';
        try {
            Class argsClass[] = {char.class, char.class};
            Object args[] = {ch, ch1};
            Method m = String.class.getMethod("replace", argsClass);//	replace(char oldChar, char newChar)
            String str = (String) m.invoke(string, args);
            //System.out.println(str);
            if (str.contains("F")) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test44(String string) {
        try {
            Class argsClass[] = {String.class, String.class};
            Object args[] = {"abc", "ABC"};
            Method m = String.class.getMethod("replaceAll", argsClass);//replaceAll(String regex, String replacement)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if (result.contains("ABC")) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test45(String string) {
        try {
            Class argsClass[] = {String.class, String.class};
            Object args[] = {"abc", "ABC"};
            Method m = String.class.getMethod("replaceFirst", argsClass);//	replaceFirst(String regex, String replacement)
            String str = (String) m.invoke(string, args);
            //System.out.println(str);
            if (str.contains("ABC")) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test46(String string) {
        try {
            Class argsClass[] = {String.class, int.class};
            Object args[] = {"b", 2};
            Method m = String.class.getMethod("split", argsClass);//split(String regex, int limit)
            String[] result = (String[]) m.invoke(string, args);
            //System.out.println(result.length);
            if (result.length > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test47(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"b"};
            Method m = String.class.getMethod("split", argsClass);//split(String regex)
            String[] result = (String[]) m.invoke(string, args);
            //System.out.println(result.length);
            if (result.length > 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test48(String string) {
        try {
            Class argsClass[] = {String.class};
            Object args[] = {"a"};
            Method m = String.class.getMethod("startsWith", argsClass);//	startsWith(String prefix)
            boolean result = (boolean) m.invoke(string, args);
            //System.out.println(result);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test49(String string) {
        try {
            Class argsClass[] = {String.class, int.class};
            Object args[] = {"b", 1};
            Method m = String.class.getMethod("startsWith", argsClass);//startsWith(String prefix, int toffset)
            boolean result = (boolean) m.invoke(string, args);
            //System.out.println(result);
            if (result) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test50(String string) {
        try {
            Class argsClass[] = {int.class, int.class};
            Object args[] = {0, 3};
            Method m = String.class.getMethod("subSequence", argsClass);//subSequence(int beginIndex, int endIndex)
            CharSequence result = (CharSequence) m.invoke(string, args);
            //System.out.println(result);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test51(String string) {
        try {
            Class argsClass[] = {int.class, int.class};
            Object args[] = {0, 3};
            Method m = String.class.getMethod("substring", argsClass);//substring(int beginIndex, int endIndex)
            String result = (String) m.invoke(string, args);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test52(String string) {
        try {
            Class argsClass[] = {int.class};
            Object args[] = {0};
            Method m = String.class.getMethod("substring", argsClass);//substring(int beginIndex)
            String result = (String) m.invoke(string, args);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            //e.printStackTrace();
            processResult -= 2;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test53(String string) {
        try {
            Method m = String.class.getMethod("toCharArray");//toCharArray()
            char[] result = (char[]) m.invoke(string);
            //System.out.println(result.length);
            if (result.length >= 0) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test54(String string) {
        try {
            Method m = String.class.getMethod("toLowerCase");//toLowerCase()
            String result = (String) m.invoke(string);
            //System.out.println(result);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test55(String string) {
        try {
            Class argsClass[] = {Locale.class};
            Object args[] = {Locale.getDefault()};
            Method m = String.class.getMethod("toLowerCase", argsClass);//toLowerCase(Locale locale)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test56(String string) {
        try {
            Method m = String.class.getMethod("toString");//toString()
            String result = (String) m.invoke(string);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test57(String string) {
        try {
            Class argsClass[] = {Locale.class};
            Object args[] = {Locale.getDefault()};
            Method m = String.class.getMethod("toUpperCase", argsClass);//toUpperCase(Locale locale)
            String result = (String) m.invoke(string, args);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test58(String string) {
        try {
            Method m = String.class.getMethod("toUpperCase");//toUpperCase()
            String result = (String) m.invoke(string);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test59(String string) {
        try {
            Method m = String.class.getMethod("trim");//trim()
            String result = (String) m.invoke(string);
            if (result != null) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test60(String string) {
        try {
            Class argsClass[] = {boolean.class};
            Object args[] = {true};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(boolean b)
            String result = (String) m.invoke(string, args);
            if ("true".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test61(String string) {
        Double d = new Double(11.23);
        try {
            Class argsClass[] = {double.class};
            Object args[] = {d};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(double d)
            String result = (String) m.invoke(null, args);
            //System.out.println(result);
            if ("11.23".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test62(String string) {
        char[] chars = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
        try {
            Class argsClass[] = {char[].class, int.class, int.class};
            Object args[] = {chars, 1, 3};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(char[] data, int offset, int count)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if ("bcd".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test63(String string) {
        float f = (float) 11.0;
        try {
            Class argsClass[] = {float.class};
            Object args[] = {f};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(float f)
            String result = (String) m.invoke(string, args);
            if ("11.0".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test64(String string) {
        try {
            Class argsClass[] = {int.class};
            Object args[] = {1};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(int i)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if ("1".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test65(String string) {
        try {
            Class argsClass[] = {char.class};
            Object args[] = {'a'};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(char c)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if ("a".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test66(String string) {
        long l = 1000000000;
        try {
            Class argsClass[] = {long.class};
            Object args[] = {l};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(long l)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if ("1000000000".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
    private static void test67(String string) {
        try {
            Class argsClass[] = {Object.class};
            Object args[] = {string};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(Object obj)
            String result = (String) m.invoke(string, args);
            //System.out.println(result);
            if (string.equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            //se.printStackTrace();
            processResult -= 2;
        }
    }
    private static void test68(String string) {
        char[] chars = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
        try {
            Class argsClass[] = {char[].class};
            Object args[] = {chars};
            Method m = String.class.getMethod("valueOf", argsClass);//valueOf(char[] data)
            String result = (String) m.invoke(string, args);
            if ("abcdefg".equals(result)) {
                processResult -= 2;
            }
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NullPointerException se) {
            se.printStackTrace();
        }
    }
}