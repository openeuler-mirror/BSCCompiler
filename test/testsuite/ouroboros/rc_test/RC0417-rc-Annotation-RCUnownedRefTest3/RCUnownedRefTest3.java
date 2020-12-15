/*
 *- @TestCaseID:maple/runtime/rc/annotation/RCUnownedTest3
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:在匿名类和lambda中使用@Unowned，没有环泄露
 *- @Condition:no
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 在匿名类和lambda中使用@Unowned，没有环泄露
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCUnownedRefTest3.java
 *- @ExecuteClass: RCUnownedRefTest3
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */
import com.huawei.ark.annotation.Unowned;

import java.util.LinkedList;
import java.util.List;

public class RCUnownedRefTest3 {
    public static void main(String[] args) {
        UnownedAnnoymous unownedAnnoymous = new UnownedAnnoymous();
        unownedAnnoymous.anonymousCapture("test");
        UnownedLambda unownedLambda = new UnownedLambda();
        List<String> list = new LinkedList<>();
        list.add("test");
        unownedLambda.lambdaCapture(list, "test");
        if (unownedAnnoymous.checkAnnoy == true && unownedLambda.checkLambda == true) {
            System.out.println("ExpectResult");
        }
    }
}

class UnownedLambda {
    String name;
    static boolean checkLambda = false;
    
    void lambdaCapture(List<String> list, String suffix) {
        final @Unowned String capSuffix = suffix;
        final @Unowned UnownedLambda self = this;
        list.forEach((str) -> {
            if ((self.name + str + capSuffix).equals("nulltesttest")) {
                checkLambda = true;
            }
        });
    }
}

class UnownedAnnoymous {
    String name;
    static boolean checkAnnoy = false;

    void anonymousCapture(String suffix) {
        final @Unowned String capSuffix = suffix;
        Runnable r = new Runnable() {
            @Override
            public void run() {
                if ((name + capSuffix).equals("nulltest")) {
                    checkAnnoy = true;
                }
            }
        };
        r.run();
    }
}




// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
