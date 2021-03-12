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


import java.lang.reflect.Method;
public class MethodMultiThreadTest {
    volatile static boolean keepRunning = true;
    static int passCnt = 0;
    static int executeCnt = 0;
    public static void main(String[] args) {
        try {
            methodMultithread();
            System.out.println(passCnt/executeCnt - 28);
        } catch (Exception e) {
            System.out.println(e);
        }
    }
    private static int methodMultithread() throws Exception {
        Class clazz = Class.forName("TestMultiThreadM");
        Thread writer = new Thread(() -> {
            try {
                while (keepRunning) {
                    for (Method m: clazz.getDeclaredMethods()) {
                        m.setAccessible(true);
                        Method method = clazz.getDeclaredMethod(m.getName(), m.getParameterTypes());
                        method.setAccessible(true);
                    }
                    for (Method m: clazz.getMethods()) {
                        m.setAccessible(true);
                        Method method = clazz.getMethod(m.getName(), m.getParameterTypes());
                        method.setAccessible(true);
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
                System.out.println(e);
            }
        });
        Thread reader = new Thread(() -> {
            try {
                while (keepRunning) {
                    for (Method m: clazz.getDeclaredMethods()) {
                        passCnt += m.isAccessible()== false ? 1 : 0;
                        Method method = clazz.getDeclaredMethod(m.getName(), m.getParameterTypes());
                        passCnt += method.isAccessible()== false ? 1 : 0;
                    }
                    for (Method m: clazz.getMethods()) {
                        passCnt += m.isAccessible()== false ? 1 : 0;
                        Method method = clazz.getMethod(m.getName(), m.getParameterTypes());
                        passCnt += method.isAccessible()== false ? 1 : 0;
                    }
                    executeCnt++;
                }
            } catch (Exception e) {
                e.printStackTrace();
                System.out.println(e);
            }
        });
        writer.start();
        reader.start();
        Thread.sleep(100);
        keepRunning = false;
        Thread.sleep(100);
        writer.join();
        reader.join();
        return passCnt;
    }
}
class TestMultiThreadM {
    public char charPubMethod(int... a) {
        return 'a';
    }
    private int intPriMethod() {
        return 416;
    }
    protected int[] intProMethod(Double... b) {
        return new int[] {123};
    }
    String stringMethod() {
        return "hey, hello";
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
