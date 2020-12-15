/*
 *- @TestCaseID:maple/runtime/rc/function/PhantomRefTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: PhantomRefTest basic testcase
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: PhantomRefTest.java
 *- @ExecuteClass: PhantomRefTest
 *- @ExecuteArgs:
 *- @Remark:
 *
 */
import java.lang.ref.*;

public class PhantomRefTest {
  static Reference rp;
  static ReferenceQueue rq = new ReferenceQueue();
  static int a = 100;

  static void setPhantomReference() {
    rp = new PhantomReference<Object>(new Object(), rq);
    if (rp.get() != null) {
      a++;
    }
  }

  public static void main(String [] args) throws Exception {
    Reference r;
    setPhantomReference();
    Thread.sleep(2000);
    while ((r = rq.poll())!=null) {
      if (!r.getClass().toString().equals("class java.lang.ref.PhantomReference")) {
        a++;
      }
    }

    if (a == 100) {
      System.out.println("ExpectResult");
    }
  }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
