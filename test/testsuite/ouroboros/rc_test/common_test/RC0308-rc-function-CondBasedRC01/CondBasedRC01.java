/*
 *- @TestCaseID:maple/runtime/rc/function/CondBasedRC01.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test condbasedrc for RC, decref can be omitted if ref is null can be guaranteed
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: CondBasedRC01.java
 *- @ExecuteClass: CondBasedRC01
 *- @ExecuteArgs:
 *- @Remark:
 */
public class CondBasedRC01 {
    private String str1;
    public static String str2;

    public CondBasedRC01() {
    }

    public String testfunc(String s) {
        if (s == null) {
            s = str1;
        }
        return s;
    }

    public String strfunc() {
        return str1;
    }

    public String testfunc1() {
        String t = strfunc();
        if (t == null) {
            t = str1;
        }
        return t;
    }

    public static void main(String[] args){
        CondBasedRC01 temp1 = new CondBasedRC01();
        String s = new String("test");
        str2 = temp1.testfunc(s);
        str2 = temp1.testfunc1();
        System.out.println("ExpectResult");
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
