/*
 *- @TestCaseID: ConstructorMultiThreadTest.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ConstructorMultiThreadTest.java
 *- @Title/Destination: when one thread changes the accessiblity of constructor by Constructor object A, the other thread will not know if it access the constructor by another Constructor object B
 *- @Condition: no
 *- @Brief:no:
 * -#step1. ConstructorMultiThreadTest have constructors with different access modifier(public/protected/default/private), thread A keep setAccessible(true) for Constructor object of these constructors
 * -#step2. thread B get new Constructor object for same constructor in ConstructorMultiThreadTest, and verify isAccessible() return false.
 *- @Expect:0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ConstructorMultiThreadTest.java
 *- @ExecuteClass: ConstructorMultiThreadTest
 *- @ExecuteArgs:
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
