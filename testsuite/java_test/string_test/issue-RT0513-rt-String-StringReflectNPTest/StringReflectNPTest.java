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
import java.lang.reflect.*;
public class StringReflectNPTest {
    private static int processResult = 99;
    public static void main(String argv[]) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[], PrintStream out) {
        int result = 2/*STATUS_FAILED*/;
        try {
            StringReflectNPTest_1();
        } catch (Exception e) {
            processResult -= 10;
        }
//        System.out.println(result);
//        System.out.println(StringReflectNPTest.res);
        if (result == 2 && processResult == 85) {
            result = 0;
        }
        return result;
    }
    public static void StringReflectNPTest_1() {
        int result1 = 4; /*STATUS_FAILED*/
        String str1_1 = new String("abc123");
        String str1_2 = new String("      ");
        String str1_3 = new String("abc123");
        String str1_4 = new String("");
        String str1_5 = new String();
        String str2_1 = "abc123";
        String str2_2 = "      ";
        String str2_3 = "abc123";
        String str2_4 = "";
        String str2_5 = null;
        test1(str2_5);
        test2(str2_5);
        test3(str2_5);
        test4(str2_5);
        test5(str2_5);
        test6(str2_5);
        test7(str2_5);
    }
    private static void test1(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {String.class};
            Object args[] = {0};
            Method m = String.class.getMethod("charAt", params);
            Object result = m.invoke(str, args);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail1-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
    private static void test2(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {String.class};
//            Object args[] = {0};
            Method m = String.class.getMethod("toCharArray");
            Object result = m.invoke(str);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail2-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
    private static void test3(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {String.class};
            Object args[] = {1};
            Method m = String.class.getMethod("substring", params);
            Object result = m.invoke(str, args);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail3-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
    private static void test4(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {String.class};
            Object args[] = {new String("aaa")};
            Method m = String.class.getMethod("compareTo", argsClass);
            Object result = m.invoke(str, args);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail4-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
    private static void test5(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {String.class};
//            Object args[] = {0};
            Method m = String.class.getMethod("intern");
            Object result = m.invoke(str);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail5-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
    private static void test6(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {char.class, char.class};
            Object args[] = {new String("a"), new String("b")};
            Method m = String.class.getMethod("replace", argsClass);
            Object result = m.invoke(str, args);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail6-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
    private static void test7(String str) {
        try {
            Class params[] = {int.class};
            Class argsClass[] = {String.class};
            Object args[] = {new String("aaa")};
            Method m = String.class.getMethod("concat", argsClass);
            Object result = m.invoke(str, args);
        } catch (NoSuchMethodException e) {
            System.out.println("===============fail7-1");
        } catch (InvocationTargetException e) {
            System.out.println("===============fail2");
        } catch (IllegalAccessException e) {
            System.out.println("===============fail3");
        } catch (NullPointerException se) {
            processResult -= 2;
        }
    }
}
