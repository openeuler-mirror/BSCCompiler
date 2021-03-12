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
import java.lang.reflect.Method;
public class MethodInvokeExceptionTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    private static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED*/

        try {
            invokeMethod();
        } catch (Exception e) {
            processResult--;
            System.out.println(3);
        }
        try {
            invokeMethodTest();
        } catch (Exception e) {
            processResult--;
            System.out.println(6);
        }
        if (result == 2 && processResult == 97) {
            result = 0;
        }
        return result;
    }
    static void invokeMethod() throws Exception {
        Method method = Class.forName("java.lang.Class").getDeclaredMethod("getConstructor", Class[].class);
        Object instance = Class.forName("java.security.CodeSigner");
        Object[] parameters = new Object[1];
        parameters[0] =
                new Class[] {
                    java.security.CodeSigner.class,
                    java.util.concurrent.locks.AbstractOwnableSynchronizer.class,
                    java.util.concurrent.locks.AbstractOwnableSynchronizer.class,
                    java.util.LinkedHashMap.class,
                    java.nio.channels.AsynchronousFileChannel.class,
                    java.sql.Time.class,
                    java.util.EnumMap.class,
                    java.nio.channels.FileLock.class,
                    java.util.WeakHashMap.class
                };
        System.out.println(1);
        method.invoke(instance, parameters);
        System.out.println(2);
    }
    static void invokeMethodTest() throws Exception {
        Method method =
                Class.forName("java.lang.Character").getDeclaredMethod("CharConversionException", Class[].class);
        Object instance = Class.forName("java.security.CodeSigner");
        Object[] parameters = new Object[1];
        parameters[0] =
                new Class[] {
                    java.security.CodeSigner.class,
                    java.util.concurrent.locks.AbstractOwnableSynchronizer.class,
                    java.util.concurrent.locks.AbstractOwnableSynchronizer.class,
                    java.util.LinkedHashMap.class,
                    java.nio.channels.AsynchronousFileChannel.class,
                    java.sql.Time.class,
                    java.util.EnumMap.class,
                    java.nio.channels.FileLock.class,
                    java.util.WeakHashMap.class
                };
        System.out.println(4);
        method.invoke(instance, parameters);
        System.out.println(5);
    }
}
