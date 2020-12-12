/*
 *- @TestCaseID: Maple_Annotation_invoke_AnnotationSetTest
 *- @RequirementName: Java Reflection
 *- @RequirementID: SR000B7N9H
 *- @TestCaseName:ReflectClassNameTest.java
 *- @TestCaseName: ReflectClassNameTest
 *- @TestCaseType: Function Testing
 *- @RequirementID: AR000D0OR6
  *- @RequirementName: annotation
 *- @Title/Destination: positive test for reflection, able to get class, method, field with non-english
 *- @Condition: no
 *- @Brief:no:
 * -#step1. class, method, field with non-english name
 * -#step2. able to be get by reflection
 *- @Expect:0\n
 *- @Priority: Level 1
 *- @Remark:
 *- @Source: ReflectClassNameTest.java
 *- @ExecuteClass: ReflectClassNameTest
 *- @ExecuteArgs:
 */
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;

public class ReflectClassNameTest {
    static int[] passCnt = new int[6];
    public static void main(String[] args) throws ClassNotFoundException, NoSuchMethodException, NoSuchFieldException {
        int i = 0;
        Class clazz = Class.forName("一");
        passCnt[i++] = clazz.getName().compareTo("一") == 0 ? 1 : 0;
        Method method = clazz.getDeclaredMethod("дракон");
        passCnt[i++] = method.getName().compareTo("дракон") == 0 ? 1 : 0;
        Field field = clazz.getDeclaredField("Rồng");
        passCnt[i++] = field.getName().compareTo("Rồng") == 0 ? 1 : 0;

        Class clazz2 = Class.forName("二");
        passCnt[i++] = clazz2.getName().compareTo("二") == 0 ? 1 : 0;

        class 三{

        }
        Class clazz3 = Class.forName("ReflectClassNameTest$1三");
        passCnt[i++] = clazz3.getName().compareTo("ReflectClassNameTest$1三") == 0 ? 1 : 0;

        Class clazz4 = Class.forName("ReflectClassNameTest$二");
        passCnt[i++] = clazz4.getName().compareTo("ReflectClassNameTest$二") == 0 ? 1 : 0;

        System.out.println(Arrays.toString(passCnt).compareTo("[1, 1, 1, 1, 1, 1]"));
    }

    class 二{

    }
}

class 一{
    int Rồng;

    public void дракон(){

    }
}

interface 二{

}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
