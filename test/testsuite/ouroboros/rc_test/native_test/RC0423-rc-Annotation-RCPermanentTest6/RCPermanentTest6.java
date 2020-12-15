/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentTest6
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:@Permanent &@Unowned 在同个case中使用。添加Permanent annotation的数组，验证是否经过RC策略以及是否为堆内存；添加Unowned annotation，确认被正常释放
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 添加Permanent annotation的普通数组,验证是否经过RC策略以及是否为堆内存
 * 添加Unowned annotation，确认被正常释放
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RCPermanentTest6.java jniTestHelper.cpp
 *- @ExecuteClass: RCPermanentTest6
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */

import java.lang.reflect.Field;
import java.util.ArrayList;

import com.huawei.ark.annotation.*;

import java.lang.ref.WeakReference;

public class RCPermanentTest6 {
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

class Test_B {
    @Unowned
    Test_A aa;

    // add volatile will crash
    // static Test_A a1;

    protected void finalize() {
        System.out.println("ExpectResult");
    }
}

class Test_A {
    Test_B bb;

    public void test() {
        setReferences();
        System.runFinalization();
    }

    private void setReferences() {
        Test_A ta = new Test_A();
        ta.bb = new Test_B();
        //ta.bb.aa = ta;

        try {
            Field m = Test_B.class.getDeclaredField("aa");
            m.set(ta.bb, ta);
            Test_A a_temp = (Test_A) m.get(ta.bb);
            if (a_temp != ta) {
                System.out.println("error");
            }
            //Field m1 = Test_B.class.getDeclaredField("a1");
            //m1.set(null, ta);
        } catch (Exception e) {
            System.out.println(e);
        }
    }
}
// DEPENDENCE: jniTestHelper.cpp
// EXEC:%maple  %f jniTestHelper.cpp %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
