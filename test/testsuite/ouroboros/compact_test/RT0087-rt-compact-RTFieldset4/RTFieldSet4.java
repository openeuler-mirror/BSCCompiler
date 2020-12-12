/*
 *- @TestCaseID: RTFieldSet4
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTFieldSet4.java
 *- @Title/Destination: To set a value with no corresponding object on a static field by reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义含私有变量的类FieldSet4。
 * -#step2: 通过调用forName()方法加载类FieldSet4，调用newInstance()方法生成实例对象。
 * -#step3: 通过调用getDeclaredField()获取对应名称的成员变量，调用set()方法设置变量的值。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTFieldSet4.java
 *- @ExecuteClass: RTFieldSet4
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;

class FieldSet4 {
    public static String str;
    private int num = 1;
    public boolean aBoolean = true;
    public int num1 = 8;
}

public class RTFieldSet4 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldSet4");
            Object obj = cls.newInstance();
            Field field = cls.getDeclaredField("str");
            field.set(null, "aaa");
            System.out.println(0);
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (InstantiationException e2) {
            System.err.println(e2);
            System.out.println(2);
        } catch (NoSuchFieldException e3) {
            System.err.println(e3);
            System.out.println(2);
        } catch (IllegalAccessException e4) {
            System.err.println(e4);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
