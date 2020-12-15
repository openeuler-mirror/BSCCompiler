/*
 *- @TestCaseID:maple/runtime/rc/function/WeakRefTest.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: WeakRefTest basic testcase
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: WeakRefTest.java
 *- @ExecuteClass: WeakRefTest
 *- @ExecuteArgs:
 *- @Remark:
 *
 */
import java.lang.ref.*;

public class WeakRefTest {
  static Reference rp;
  static ReferenceQueue rq = new ReferenceQueue();
  static int a = 100;

  static void setWeakReference() {
    rp = new WeakReference<Object>(new Object(), rq);
    if (rp.get() == null) {
      a++;
    }
  }

  static class TriggerRP implements Runnable {
    public void run() {
      for (int i = 0; i < 60; i++) {
        WeakReference wr = new WeakReference(new Object());
        try {
          Thread.sleep(50);
        } catch (Exception e) {}
      }
    }
  }

  public static void main(String [] args) throws Exception {
    Reference r;
    setWeakReference();
    new Thread(new TriggerRP()).start();
    Thread.sleep(3000);
    if (rp.get() != null) {
      a++;
    }
    while ((r = rq.poll())!=null) {
      if (!r.getClass().toString().equals("class java.lang.ref.WeakReference")) {
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
