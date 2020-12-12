/*
 *- @TestCaseID: RTConstructorGetDeclaredAnnotations1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTConstructorGetDeclaredAnnotations1.java
 *- @Title/Destination: Use Constructor.getDeclaredAnnotations() To obtain all annotations for constructor of the target
 *                      class by reflection.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 定义两个类，有父子关系，都含有注解的构造方法。
 * -#step2：调用getConstructor(Class...<?> parameterTypes)从子类中获取有两个注解的构造方法。
 * -#step3：调用getDeclaredAnnotations()获取注解数组，判断数组长度为2。
 * -#step4：调用getConstructor(Class...<?> parameterTypes)从子类中获取有一个注解的构造方法。
 * -#step5：调用getDeclaredAnnotations()获取注解数组，判断数组长度为1，获取注解成员正确。
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTConstructorGetDeclaredAnnotations1.java
 *- @ExecuteClass: RTConstructorGetDeclaredAnnotations1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Constructor;

@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    int i_a() default 2;
    String t_a() default "";
}

class ConstructorGetDeclaredAnnotations1 {
    public ConstructorGetDeclaredAnnotations1() {
    }

    @IF1(i = 333, t = "test1")
    public ConstructorGetDeclaredAnnotations1(String name) {
    }

    @IF1(i = 333, t = "test1")
    ConstructorGetDeclaredAnnotations1(int number) {
    }
}

class ConstructorGetDeclaredAnnotations1_a extends ConstructorGetDeclaredAnnotations1 {
    @IF1_a(i_a = 666, t_a = "right1")
    @IF1(i = 333, t = "test1")
    public ConstructorGetDeclaredAnnotations1_a(String name) {
    }

    @IF1_a(i_a = 666, t_a = "right1")
    ConstructorGetDeclaredAnnotations1_a(int number) {
    }
}

public class RTConstructorGetDeclaredAnnotations1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetDeclaredAnnotations1_a");
            Constructor instance1 = cls.getConstructor(String.class);
            if (instance1.getDeclaredAnnotations().length == 2) {
                Constructor instance2 = cls.getDeclaredConstructor(int.class);
                if (instance2.getDeclaredAnnotations().length == 1) {
                    Annotation[] j = instance2.getDeclaredAnnotations();
                    if (j[0].toString().indexOf("i_a=666") != -1 && j[0].toString().indexOf("t_a=right1") != -1) {
                        System.out.println(0);
                        return;
                    }
                }
                System.out.println(1);
                return;
            }
            System.out.println(2);
            return;
        } catch (ClassNotFoundException e) {
            System.out.println(3);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(4);
            return;
        } catch (NullPointerException e2) {
            System.out.println(5);
            return;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
