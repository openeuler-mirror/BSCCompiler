/*
 *- @TestCaseID:maple/runtime/rc/function/SoftRefTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: SoftRefTest basic testcase
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: SoftRefTest.java
 *- @ExecuteClass: SoftRefTest
 *- @ExecuteArgs:
 *- @Remark:
 *
 */
import java.lang.ref.*;

public class SoftRefTest {
  static Reference rp;
  static ReferenceQueue rq = new ReferenceQueue();
  static int a = 100;

  static void setSoftReference() {
    rp = new SoftReference<Object>(new Object(), rq);
    if (rp.get() == null) {
      a++;
    }
  }

  static class TriggerRP implements Runnable {
    public void run() {
      for (int i = 0; i < 60; i++) {
        SoftReference sr = new SoftReference(new Object());
        try {
          Thread.sleep(50);
        } catch (Exception e) {}
      }
    }
  }

  public static void main(String [] args) throws Exception {
    Reference r;
    setSoftReference();
    new Thread(new TriggerRP()).start();
    Thread.sleep(3000);
    if (rp.get() != null) {
      a++;
    }
    while ((r = rq.poll())!=null) {
      if(!r.getClass().toString().equals("class java.lang.ref.SoftReference")) {
        a++;
      }
    }
    if (a == 101) {
      System.out.println("ExpectResult");
    }
  }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
