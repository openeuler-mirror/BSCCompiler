/*
 *- @TestCaseID: RTMethodGetDefaultValue1
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:RTMethodGetDefaultValue1.java
 *- @Title/Destination: Method.getDefaultValue() Returns the default value for the annotation member represented by this
  *                     Method instance. IF no default, return null.
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法分别获取IF1_a类、IF1类、MethodGetDefaultValue1类的运行时类clazz1、clazz2、clazz3；
 * -#step2: 分别以i_a、t为参数，通过getDeclaredMethod()方法分别获取clazz1、clazz2的声明方法并记为method1、method3；
 * -#step3: 以t_a为参数，通过getMethod()方法获取clazz1的方法对象并记为method2；
 * -#step4: 通过getMethods()方法获取clazz3的所有的方法对象并记为methods；
 * -#step5: 分别以(tt、int.class)和s为参数，通过getDeclaredMethod()方法分别获取clazz3、clazz1的声明方法并记为method5、
 *          method6；
 * -#step6: 经判断得知method1.getDefaultValue()的返回值为2，method2.getDefaultValue().toString()的返回值与字符串
 *          "string default value"相同；
 * -#step7: 经判断得知method3.getDefaultValue()的返回值为空；
 * -#step8: 经判断得知methods[0].getDefaultValue()、method5.getDefaultValue()的返回值均为null；
 * -#step9: 经判断得知method6.getDefaultValue()的返回值为null；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: RTMethodGetDefaultValue1.java
 *- @ExecuteClass: RTMethodGetDefaultValue1
 *- @ExecuteArgs:
 */

import java.lang.annotation.*;
import java.lang.reflect.Method;

@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i() default 0;
    String t() default "";
}

@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    public int i_a() default 2;
    String t_a() default "string default value";
    String s();
}

class MethodGetDefaultValue1 {
    @IF1(i = 333, t = "test1")
    public static void ii(String name) {
    }

    void tt(int number) {
    }
}

public class RTMethodGetDefaultValue1 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("IF1_a");
            Class clazz2 = Class.forName("IF1");
            Class clazz3 = Class.forName("MethodGetDefaultValue1");
            Method method1 = clazz1.getDeclaredMethod("i_a");
            Method method2 = clazz1.getMethod("t_a");
            Method method3 = clazz2.getDeclaredMethod("t");
            Method[] methods = clazz3.getMethods();
            Method method5 = clazz3.getDeclaredMethod("tt", int.class);
            Method method6 = clazz1.getDeclaredMethod("s");
            if ((int) method1.getDefaultValue() == 2
                    && method2.getDefaultValue().toString().equals("string default value")) {
                if (method3.getDefaultValue().equals("")) {
                    if (methods[0].getDefaultValue() == null && method5.getDefaultValue() == null) {
                        if (method6.getDefaultValue() == null) {
                            System.out.println(0);
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
