/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentTest7
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:@Permanent &@Weak在同个case中使用。添加Permanent annotation的数组，验证是否经过RC策略以及是否为堆内存；添加Weak annotation，确认被正常释放
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 添加Permanent annotation的普通数组,验证是否经过RC策略以及是否为堆内存
 * 添加Weak annotation，确认被正常释放
 *- @Expect:ExpectResult\nExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RCPermanentTest7.java jniTestHelper.cpp
 *- @ExecuteClass: RCPermanentTest7
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */

import com.huawei.ark.annotation.*;


public class RCPermanentTest7 {
    public static native boolean isHeapObject(Object obj);

    public static native int refCount(Object obj);

    public static int checkNum = 0;

    static Object owner;

    static boolean checkRC(Object obj) {
        int rc1 = refCount(obj);
        owner = obj;
        int rc2 = refCount(obj);
        owner = null;
        int rc3 = refCount(obj);
        if (rc1 != rc3) {
            throw new RuntimeException("rc incorrect!");
        }
        if (rc1 == rc2 && rc2 == rc3) {
            //如果相等，说明annotation生效，没有经过RC处理
            return false;
        } else {
            return true;
        }
        //System.out.printf("rc:%-5s heap:%-5s %s%n", !skipRC, isHeapObject(obj), title);
    }

    /*
    验证new int @Permanent [8]
     */
    static void method1(Object obj) {
        obj = new int@Permanent[8];
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new int @Permanent [8];in method1");
        }
    }


    public static void main(String[] args) {
        Object obj = null;
        method1(obj);
        new Test_A().test();
        if (checkNum == 1) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("checkNum:" + checkNum);
        }
    }
}

class Test_A {
    @Weak
    Test_B bb;
    Test_B bb2;

    public void test() {
        foo();
        try {
            Thread.sleep(5000);
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            bb.run();
        } catch (NullPointerException e) {
            System.out.println("ExpectResult");
        }
        bb2.run();
    }

    private void foo() {
        bb = new Test_B();
        bb2 = new Test_B();
    }
}

class Test_B {
    public void run() {
        System.out.println("ExpectResult");
    }
}
// DEPENDENCE: jniTestHelper.cpp
// EXEC:%maple  %f jniTestHelper.cpp %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\nExpectResult\n
