/*
 *- @TestCaseID: RTFieldToGenericString
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTFieldToGenericString.java
 *- @Title/Destination: Field.toGenericString() Returns a string describing this Field, including its generic type.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义含私有变量的类FieldToGenericString。
 * -#step2: 通过调用forName()方法加载类FieldToGenericString，调用newInstance()方法生成实例对象。
 * -#step3: 通过调用toGenericString()返回对象的string，确认返回的字符串描述正确。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTFieldToGenericString.java
 *- @ExecuteClass: RTFieldToGenericString
 *- @ExecuteArgs:
 */

import java.lang.reflect.Field;
import java.util.List;

class FieldToGenericString<E, F> {
    public List<F> list1;
    private List<E> list2;
}

public class RTFieldToGenericString {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldToGenericString");
            Field instance1 = cls.getField("list1");
            Field instance2 = cls.getDeclaredField("list2");
            String q1 = instance1.toGenericString();
            String q2 = instance2.toGenericString();
            if (q1.equals("public java.util.List<F> FieldToGenericString.list1") && q2.equals
                    ("private java.util.List<E> FieldToGenericString.list2")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
