/*
 *- @TestCaseID: FieldMultiThreadTest.java
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:FieldMultiThreadTest.java
 *- @Title/Destination: when one thread changes the accessiblity of field by Field object A, the other thread will not know if it access the field by another Field object B
 *- @Condition: no
 *- @Brief:no:
 * -#step1. TestMultiThread have fields with different access modifier(public/protected/default/private), thread A keep setAccessible(true) for Field object of these fields
 * -#step2. thread B get new Field object for same field in TestMultiThread, and verify isAccessible() return false.
 *- @Expect:0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: FieldMultiThreadTest.java
 *- @ExecuteClass: FieldMultiThreadTest
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

public class FieldMultiThreadTest {
    volatile static boolean keepRunning = true;
    static int passCnt = 0;
    static int executeCnt = 0;

    public static void main(String[] args) {
        try {
            fieldMultithread();
            System.out.println(passCnt/executeCnt - 10);
        } catch (Exception e) {
            System.out.println(e);
        }
    }

    private static int fieldMultithread() throws Exception {
        Class clazz = Class.forName("TestMultiThread");
        String[] allFields = new String[] {"charPub", "fieldIntPri", "fieldIntPro", "fieldString"};
        Thread writer = new Thread(() -> {
            try {
                while (keepRunning) {
                    for (String s : allFields) {
                        Field field = clazz.getDeclaredField(s);
                        field.setAccessible(true);
                    }
                    for (Field f: clazz.getDeclaredFields()) {
                        f.setAccessible(true);
                    }
                    Field field = clazz.getField("charPub");
                    field.setAccessible(true);
                    for (Field f: clazz.getFields()) {
                        f.setAccessible(true);
                    }
                }
            } catch (Exception e) {
                System.out.println(e);
            }
        });

        Thread reader = new Thread(() -> {
            try {
                while (keepRunning) {
                    for (String s : allFields) {
                        Field field = clazz.getDeclaredField(s);
                        passCnt += field.isAccessible()== false ? 1 : 0;
                    }
                    for (Field f: clazz.getDeclaredFields()) {
                        passCnt += f.isAccessible()== false ? 1 : 0;
                    }
                    Field field = clazz.getField("charPub");
                    passCnt += field.isAccessible()== false ? 1 : 0;
                    for (Field f: clazz.getFields()) {
                        passCnt += f.isAccessible()== false ? 1 : 0;
                    }
                    executeCnt++;
                }
            } catch (Exception e) {
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

class TestMultiThread {
    public char charPub = 'a';
    private int fieldIntPri = 416;
    protected int[] fieldIntPro = {123};
    String fieldString = "hey, hello";
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
