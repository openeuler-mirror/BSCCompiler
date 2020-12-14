/*
 * -@TestCaseID: T7040104
 * -@TestCaseName: T7040104
 * -@TestCaseType: Function Testing from JTreg
 * -@RequirementID: None
 * -@RequirementName: some testcases will be moved to FigtoTest when enable JTreg.
 * -@Condition:no
 *  -#c1: 测试环境正常
 * -@Brief:将该用例从JTreg中摘出来，放到CI和组件daily测试中测试。该用例在JTreg连跑中会挂掉
 * -@Expect:ExpectResult\n
 * -@Priority: High
 * -@Source: T7040104.java
 * -@ExecuteClass: T7040104
 * -@ExecuteArgs:
 * -@Remark:
 */

public class T7040104 {
    int npeCount = 0;

    public static void main(String[] args) {
        T7040104 t = new T7040104();
        t.test1();
        t.test2();
        t.test3();
        if (t.npeCount != 3) {
            System.out.println("error");
        } else {
            System.out.println("ExpectResult");
        }
    }

    void test1() {
        Object[] a;
        try {
            Object o = (a = null)[0];
        } catch (NullPointerException npe) {
            npeCount++;
        }
    }

    void test2() {
        Object[][] a;
        try {
            Object o = (a = null)[0][0];
        } catch (NullPointerException npe) {
            npeCount++;
        }
    }

    void test3() {
        Object[][][] a;
        try {
            Object o = (a = null)[0][0][0];
        } catch (NullPointerException npe) {
            npeCount++;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
