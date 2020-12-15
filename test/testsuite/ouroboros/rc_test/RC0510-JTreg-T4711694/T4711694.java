/*
 * -@TestCaseID: T4711694
 * -@TestCaseName: T4711694
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: T4711694.java
 * -@ExecuteClass: T4711694
 * -@ExecuteArgs:
 * -@Remark:
 */

public class T4711694 {
    public static void main(String[] args) {
        D.main(args);
        System.out.println("ExpectResult");
    }

    interface A<T> {
        void f(T u);
    }

    static class B {
        public void f(Integer i) {
        }
    }

    static abstract class C<T> extends B implements A<T> {
        public void g(T t) {
            f(t);
        }
    }

    static class D extends C<Integer> {
        public static void main(String[] args) {
            new D().g(new Integer(3));
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
