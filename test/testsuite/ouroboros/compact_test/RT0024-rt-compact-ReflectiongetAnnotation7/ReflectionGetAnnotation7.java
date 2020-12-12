/*
 *- @TestCaseID: ReflectionGetAnnotation7
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionGetAnnotation7.java
 *- @Title/Destination: Use Class.GetAnnotation to get native basic annotations through reflection.
 *- @Condition: no
 *- @Brief:no:
 *  -#step1: 创建类GetAnnotation7的子类GetAnnotation7_a且重写父类中废弃的aa()方法。
 *  -#step2: 通过forName()获取GetAnnotation7_a的类名对象。
 *  -#step3: 通过getMethod()获取GetAnnotation7_a内的方法。
 *  -#step4：调用getAnnotation(Class<T> annotationClass)获取Deprecated.class的值。
 *  -#step5：对获取的注解结果进行判断
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionGetAnnotation7.java
 *- @ExecuteClass: ReflectionGetAnnotation7
 *- @ExecuteArgs:
 */

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

class GetAnnotation7 {
    @Deprecated
    public int aa(int num) {
        return 0;
    }
}

class GetAnnotation7_a extends GetAnnotation7 {
    @Override
    public int aa(int num) {
        return 2;
    }
}

public class ReflectionGetAnnotation7 {

    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation7_a");
            Method zhu1 = zqp1.getMethod("aa", int.class);
            Annotation[] j = zhu1.getAnnotations();
            if (j.length == 0) {
                try {
                    Class zqp2 = Class.forName("GetAnnotation7");
                    Method zhu2 = zqp2.getMethod("aa", int.class);
                    if (zhu2.getAnnotation(Deprecated.class).toString().indexOf("Deprecated") != -1) {
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
