/*
 *- @TestCaseID:maple/runtime/rc/function/RC_Finalize_01.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test Finalizer for RC .
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RC_Finalize_01.java
 *- @ExecuteClass: RC_Finalize_01
 *- @ExecuteArgs:
 *- @Remark:
 */
class C {
    static A a;
}

class A {
    B b;

    public A(B b) {

        this.b = b;

    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
//        System.out.println("A finalize");

        C.a = this;

    }

}


class B {
    int num1;
    int num2;

    public B(int num1, int num2) {
        this.num1 = num1;
        this.num2 = num2;
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
//        System.out.println("B finalize");
    }

    public int sum() {
        return num1 + num2;
    }

}

public class RC_Finalize_01 {

    public static void main(String[] args) throws Exception {
        A a = new A(new B(12, 18));
        a = null;
        //System.gc();

        Thread.sleep(5000);
		System.out.println("ExpectResult");
}
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
