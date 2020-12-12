/*
 *- @TestCaseID: RTConstructorGetExceptionTypes
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTConstructorGetExceptionTypes.java
 *- @Title/Destination: Constructor.getExceptionTypes() returns an array of Class objects that represent the types of
 *                      exceptions declared to be thrown by the underlying executable represented by this object.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义一个类含有构造方法。
 * -#step2：调用Class.forName获取相应的class.
 * -#step3: 使用class调用getDeclaredConstructor()获取相应的构造方法。
 * -#step4：使用构造方法调用getExceptionTypes()获取异常类型的Class对象数组。
 * -#step5：判断数组长度为2。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTConstructorGetExceptionTypes.java
 *- @ExecuteClass: RTConstructorGetExceptionTypes
 *- @ExecuteArgs:
 */

import java.lang.reflect.Constructor;

class ConstructorGetExceptionTypes {
    ConstructorGetExceptionTypes() throws ExceptionInInitializerError, InstantiationException {
    }
}

public class RTConstructorGetExceptionTypes {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetExceptionTypes");
            Constructor cons = cls.getDeclaredConstructor();
            Class<?>[] exClass = cons.getExceptionTypes();
            if (exClass.length == 2) {
                System.out.println(0);
                return;
            }
            System.out.println(1);
            return;
        } catch (ClassNotFoundException e1) {
            System.out.println(2);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(3);
            return;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
