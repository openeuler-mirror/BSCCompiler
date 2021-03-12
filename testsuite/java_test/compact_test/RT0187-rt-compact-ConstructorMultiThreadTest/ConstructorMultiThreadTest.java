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


import java.lang.reflect.Constructor;
public class ConstructorMultiThreadTest {
    volatile static boolean keepRunning = true;
    static int passCnt = 0;
    static int executeCnt = 0;
    public static void main(String[] args) {
        try {
            constructorMultithread();
            System.out.println(passCnt/executeCnt - 10);
        } catch (Exception e) {
            System.out.println(e);
        }
    }
    private static int constructorMultithread() throws Exception {
        Class clazz = Class.forName("TestMultiThreadC");
        Thread writer = new Thread(() -> {
            try {
                while (keepRunning) {
                    for (Constructor c: clazz.getDeclaredConstructors()) {
                        c.setAccessible(true);
                        Constructor constructor = clazz.getDeclaredConstructor(c.getParameterTypes());
                        constructor.setAccessible(true);
                    }
                    for (Constructor c: clazz.getConstructors()) {
                        c.setAccessible(true);
                        Constructor constructor = clazz.getConstructor(c.getParameterTypes());
                        constructor.setAccessible(true);
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
                    for (Constructor c: clazz.getDeclaredConstructors()) {
                        passCnt += c.isAccessible()== false ? 1 : 0;
                        Constructor constructor = clazz.getDeclaredConstructor(c.getParameterTypes());
                        passCnt += constructor.isAccessible()== false ? 1 : 0;
                    }
                    for (Constructor c: clazz.getConstructors()) {
                        passCnt += c.isAccessible()== false ? 1 : 0;
                        Constructor constructor = clazz.getConstructor(c.getParameterTypes());
                        passCnt += constructor.isAccessible()== false ? 1 : 0;
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
class TestMultiThreadC {
    public TestMultiThreadC(int... a) {
    }
    private TestMultiThreadC(String a) {
    }
    protected TestMultiThreadC(Double... a) {
    }
    TestMultiThreadC() {
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
