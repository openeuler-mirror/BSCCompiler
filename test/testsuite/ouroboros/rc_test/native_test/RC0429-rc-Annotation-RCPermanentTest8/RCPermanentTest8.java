/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentTest8
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:在try/catch中添加Permanent annotation的字段，验证是否经过RC策略以及是否为堆内存
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 在try/catch中使用注解，验证是否经过RC策略以及是否为堆内存
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCPermanentTest8.java jniTestHelper.cpp
 *- @ExecuteClass: RCPermanentTest8
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */

import java.util.ArrayList;

import com.huawei.ark.annotation.Permanent;

import java.lang.ref.WeakReference;

public class RCPermanentTest8 {
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
        try {
            obj = new int@Permanent[8];
            boolean result1 = checkRC(obj);
            if (result1 == false && isHeapObject(obj) == false) {
                checkNum++;
            } else {
                System.out.println("error in new int @Permanent [8];in method1");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    public static void main(String[] args) {

        Object obj = null;
        method1(obj);

        if (checkNum == 1) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("checkNum:" + checkNum);
        }
    }
}
// DEPENDENCE: jniTestHelper.cpp
// EXEC:%maple  %f jniTestHelper.cpp %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
