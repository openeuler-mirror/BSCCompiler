/*
 *- @TestCaseID:maple/runtime/rc/function/RC_eh_01.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test eh for RC.
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RC_eh_01.java
 *- @ExecuteClass: RC_eh_01
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.nio.charset.IllegalCharsetNameException;

public class RC_eh_01 {
    static int res = 0;

    public static void main(String argv[]) {
        run1();
		System.out.println("ExpectResult");
    }

    public static void run1() {

        try {
            new RC_eh_01().run2();
        } catch (IllegalArgumentException e) {
            RC_eh_01.res = 1;
            
        }

    }

    public  void run2() {
        Foo f1 = new Foo(1);
        Foo f2 = new Foo(2);
        run3();
        int result=f1.i+f2.i;

    }

    public static void run3() {
        try {
            Integer.parseInt("123#456");
        } catch (IllegalCharsetNameException e) {
            RC_eh_01.res = 3;
            
        }
    }

    class Foo{
        public int i;
        public Foo(int i){
            this.i=i;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
