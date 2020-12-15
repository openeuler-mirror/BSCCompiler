/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentTest3
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:添加/没有Permanent annotation的arraylist，验证是否经过RC策略以及是否为堆内存
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 添加/没有Permanent annotation的arraylist,验证是否经过RC策略以及是否为堆内存
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCPermanentTest3.java jniTestHelper.cpp
 *- @ExecuteClass: RCPermanentTest3
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */

import java.util.ArrayList;

import com.huawei.ark.annotation.Permanent;

import java.lang.ref.WeakReference;

public class RCPermanentTest3 {
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
    static void method1() {
        ArrayList list = null;
        list = new @Permanent ArrayList<>();
        boolean result1 = checkRC(list);
        boolean result2 = isHeapObject(list);
        if (String.valueOf(result1).equals("false") && String.valueOf(result2).equals("true")) {
            //if (result1 == false && result2 == true) {
            checkNum++;
        } else {
            System.out.println("error in method1");
            System.out.println("result1:" + result1 + "       is Heap:" + result2);
        }
    }

    static void method2() {
        ArrayList list = null;
        list = new ArrayList<Object>();
                list.add("test");
        boolean result1 = checkRC(list);
        boolean result2 = isHeapObject(list);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            //if (result1 == false && result2 == true) {
            checkNum++;
        } else {
            System.out.println("error in method2");
            System.out.println("result1:" + result1 + "       is Heap:" + result2);
        }
    }


    public static void main(String[] args) {

        method1();
        Runtime.getRuntime().gc();
        method1();

        method2();
        Runtime.getRuntime().gc();
        method2();

        if (checkNum == 4) {
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
