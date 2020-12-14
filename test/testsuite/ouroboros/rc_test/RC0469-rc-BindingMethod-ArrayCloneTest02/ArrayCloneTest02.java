/*
 *- @TestCaseID: Maple_MemoryManagement2.0_ArrayCloneTest02
 *- @TestCaseName: ArrayCloneTest02
 *- @TestCaseType: Function Testing for MemoryBindingMethod Test
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: 测试环境正常
 *- @Brief:clone()函数专项测试：验证数组的clone()方法的基本功能正常。
 *  -#step1:测试数组拷贝时报CloneNotSupportedException异常
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: ArrayCloneTest02.java
 *- @ExecuteClass: ArrayCloneTest02
 *- @ExecuteArgs:
 *- @Remark:
 */

public class ArrayCloneTest02 {
    private Integer id;

    public ArrayCloneTest02(Integer id) {
        this.id = id;
    }

    public Integer getId() {
        return id;
    }

    public void setId(Integer id) {
        this.id = id;
    }

    @Override
    public String toString() {
        return "CloneTest{" +
                "id=" + id +
                '}';
    }

    public static void main(String[] args) {
        try {
            ArrayCloneTest02 cloneTest = new ArrayCloneTest02(1);
            ArrayCloneTest02 cloneTest1 = (ArrayCloneTest02) cloneTest.clone();
            System.out.println("ErrorResult");
        } catch (CloneNotSupportedException e) {
            System.out.println("ExpectResult");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
