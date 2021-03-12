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
