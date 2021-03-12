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


import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
public class MethodInvokeInvocationTargetExceptionTest {
    public static void main(String[] args) {
        try {
            invokeMethod();
        } catch (InvocationTargetException e) {
            System.out.println(0);
        } catch (Exception e) {
            System.out.println(3);
        }
    }
    static void invokeMethod()
            throws NoSuchMethodException, ClassNotFoundException, IllegalAccessException, InvocationTargetException {
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
        method.invoke(instance, parameters);
        System.out.println(2);
    }
}
