/*
 *- @TestCaseID: ReflectionForName5
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectionForName5.java
 *- @Title/Destination: Use Class.forName() to get a multilevel inherited class by reflection
 *- @Condition: no
 *- @Brief:no:
 * -#step1: 通过Class.forName()方法获取ForName5类的类型clazz，且clazz和ForName5类是同一类型；
 * -#step2: 通过Class.forName()方法获取ForName55类的类型clazz2，且clazz2和ForName55类是同一类型；
 *- @Expect: 0\n
 *- @Priority: High
 *- @Remark:
 *- @Source: ReflectionForName5.java
 *- @ExecuteClass: ReflectionForName5
 *- @ExecuteArgs:
 */

interface ForName55_c {
}

class ForName55_b implements ForName55_c {
}

class ForName55_a extends ForName55_b {
}

class ForName55 extends ForName55_a {
}

class ForName5_e {
}

class ForName5_d extends ForName5_e {
}

class ForName5_c extends ForName5_d {
}

class ForName5_b extends ForName5_c {
}

class ForName5_a extends ForName5_b {
}

class ForName5 extends ForName5_a {
}

public class ReflectionForName5 {
    public static void main(String[] args) throws ClassNotFoundException {
        Class clazz = Class.forName("ForName5");
        if (clazz.toString().equals("class ForName5")) {
            Class clazz2 = Class.forName("ForName55");
            if (clazz2.toString().equals("class ForName55")) {
                System.out.println(0);
            }
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
