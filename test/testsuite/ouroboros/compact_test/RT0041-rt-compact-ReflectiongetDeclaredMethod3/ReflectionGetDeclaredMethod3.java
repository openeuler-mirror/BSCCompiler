/*
 *- @TestCaseID: ReflectionGetDeclaredMethod3
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetDeclaredMethod3.java
 *- @Title/Destination: Class.getDeclaredMethod() can return abstract methods of target class
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取GetDeclaredMethod3类的运行时类clazz；
 * -#step2: 以aa为参数，通过getDeclaredMethod()方法获取clazz的方法对象method1；同理，以bb为参数，通过getMethod()方法获取
 *          clazz的方法对象method2；
 * -#step3: 确定method1和method2获取成功，并且是GetDeclaredMethod3类的指定的运行方法；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetDeclaredMethod3.java
 *- @ExecuteClass: ReflectionGetDeclaredMethod3
 *- @ExecuteArgs:
 */

import java.lang.reflect.Method;

abstract class GetDeclaredMethod3 {
    abstract void empty1();
    abstract public int empty2();
}

public class ReflectionGetDeclaredMethod3 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetDeclaredMethod3");
            Method method1 = clazz.getDeclaredMethod("empty1");
            Method method2 = clazz.getMethod("empty2");
            if (method1.toString().equals("abstract void GetDeclaredMethod3.empty1()")
                    && method2.toString().equals("public abstract int GetDeclaredMethod3.empty2()")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        } catch (NullPointerException e3) {
            System.err.println(e3);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
