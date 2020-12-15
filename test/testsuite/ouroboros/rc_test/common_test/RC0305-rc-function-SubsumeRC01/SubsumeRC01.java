/*
 *- @TestCaseID:maple/runtime/rc/function/SubsumeRC01.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: test subsumerc for RC, return a field directly
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: SubsumeRC01.java
 *- @ExecuteClass: SubsumeRC01
 *- @ExecuteArgs:
 *- @Remark:
 *- @ExpectMplCode:
 *-  callassigned &MRT_LoadRefField_NaiveRCFast (
 *-    regread ref %2,
 *-    iaddrof ref <* <$LSubsumeRC01_3B>> 4 (regread ref %2)) { regassign ptr %1}
 *-  regassign ref %3 (regread ptr %1)
 *-  dassign %Reg0_R71 0 (regread ref %3)
 *-  intrinsiccall MPL_CLEANUP_LOCALREFVARS_SKIP (dread ref %Reg0_R71)
 *-  return (regread ref %3)
 */
public class SubsumeRC01 {
    private String str;

    public String testfunc() {
        return str;
    }

    public static void main(String[] args){
        SubsumeRC01 temp = new SubsumeRC01();
        temp.testfunc();
        System.out.println("ExpectResult");
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
