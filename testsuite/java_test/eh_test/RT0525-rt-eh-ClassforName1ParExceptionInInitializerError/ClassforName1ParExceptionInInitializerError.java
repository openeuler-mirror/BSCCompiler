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
public class ClassforName1ParExceptionInInitializerError {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    /**
     * main test fun
     * @return status code
    */

    public static int run(String[] argv, PrintStream out) {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = classForName1ParExceptionInInitializerError();
        } catch (Exception e) {
            processResult -= 20;
        }
        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }
    /**
     * Exception in Class :public static Class<?> forName(String name, boolean initialize, ClassLoader loader)
     * @return status code
     * @throws ClassNotFoundException
    */

    public static int classForName1ParExceptionInInitializerError() throws ClassNotFoundException {
        int result1 = 4; /*STATUS_FAILED*/
        // ExceptionInInitializerError - if the initialization provoked by this method fails
        //
        // public static Class<?> forName(String className)
        Class cls = Class.forName("ClassforName1ParExceptionInInitializerError");
        ClassLoader cLoader = cls.getClassLoader();
        try {
            Class cls2 = Class.forName("Cast6");
            processResult -= 10;
        } catch (ExceptionInInitializerError e1) {
            processResult--;
        }
        return result1;
    }
}
class Cast6 {
    String str1 = "str1";
    /**
     * set String method1
    */

    public static void setStr() {
        String method1 = "1";
    }
    static {
        int num = 1 / 0;
    }
}
