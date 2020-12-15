/*
 *- @TestCaseID:maple/runtime/rc/function/RC_Thread01/RCUnownedRefThreadTest3.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Change CaseRC0417 to Multi thread testcase.
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCUnownedRefThreadTest3.java
 *- @ExecuteClass: RCUnownedRefThreadTest3
 *- @ExecuteArgs:
 *- @Remark:Owner liuweiqing 00481345
 */

import com.huawei.ark.annotation.Unowned;

import java.util.LinkedList;
import java.util.List;

class RCUnownedRefTest3 extends Thread {
    private boolean checkout;

    public void run() {
        UnownedAnnoymous unownedAnnoymous = new UnownedAnnoymous();
        unownedAnnoymous.anonymousCapture("test");
        UnownedLambda unownedLambda = new UnownedLambda();
        List<String> list = new LinkedList<>();
        list.add("test");
        unownedLambda.lambdaCapture(list, "test");
        
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        if (unownedAnnoymous.checkAnnoy == true && unownedLambda.checkLambda == true) {
            checkout = true;
        } else {
            System.out.println("unownedAnnoymous.checkAnnoy:" + unownedAnnoymous.checkAnnoy);
            System.out.println("unownedLambda.checkLambda:" + unownedLambda.checkLambda);
            System.out.println("555555555555555");
            checkout = false;
        }
    }

    public boolean check() {
        return checkout;
    }
}

class UnownedLambda {
    String name;
    static boolean checkLambda;

    void lambdaCapture(List<String> list, String suffix) {
        checkLambda = false;
        final String capSuffix = suffix;
        final UnownedLambda self = this;
        list.forEach((str) -> {
            if ((self.name + str + capSuffix).equals("nulltesttest")) {
                checkLambda = true;
            } else {
                System.out.println("self.name + str + capSuffix:" + self.name + "  " + str + "  " + capSuffix);
            }
        });
    }
}

class UnownedAnnoymous {
    String name;
    static boolean checkAnnoy;

    void anonymousCapture(String suffix) {
        checkAnnoy = false;
        final @Unowned String capSuffix = suffix;
        Runnable r = new Runnable() {
            @Override
            public void run() {
                if ((name + capSuffix).equals("nulltest")) {
                    checkAnnoy = true;
                } else {
                    System.out.println("name + capSuffix:" + name + "   " + capSuffix);
                }
            }
        };
        r.run();
    }
}

public class RCUnownedRefThreadTest3 {

    public static void main (String[] args) throws InterruptedException {
        rc_testcase_main_wrapper();
    }

    private static void rc_testcase_main_wrapper() throws InterruptedException {
        RCUnownedRefTest3 rcTest1 = new RCUnownedRefTest3();
        RCUnownedRefTest3 rcTest2 = new RCUnownedRefTest3();
        RCUnownedRefTest3 rcTest3 = new RCUnownedRefTest3();
        RCUnownedRefTest3 rcTest4 = new RCUnownedRefTest3();
        RCUnownedRefTest3 rcTest5 = new RCUnownedRefTest3();
        RCUnownedRefTest3 rcTest6 = new RCUnownedRefTest3();

        rcTest1.start();
        rcTest2.start();
        rcTest3.start();
        rcTest4.start();
        rcTest5.start();
        rcTest6.start();

        try {
            rcTest1.join();
            rcTest2.join();
            rcTest3.join();
            rcTest4.join();
            rcTest5.join();
            rcTest6.join();

        } catch (InterruptedException e) {
        }
        Thread.sleep(2000);
        if (rcTest1.check() && rcTest2.check() && rcTest3.check() && rcTest4.check() && rcTest5.check() && rcTest6.check()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("rcTest1.check():" + rcTest1.check());
            System.out.println("rcTest2.check():" + rcTest2.check());
            System.out.println("rcTest3.check():" + rcTest3.check());
            System.out.println("rcTest4.check():" + rcTest4.check());
            System.out.println("rcTest5.check():" + rcTest5.check());
            System.out.println("rcTest6.check():" + rcTest6.check());
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
