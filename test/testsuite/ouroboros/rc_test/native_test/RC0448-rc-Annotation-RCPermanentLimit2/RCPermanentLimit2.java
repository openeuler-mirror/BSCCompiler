/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentTest8
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:添加Permanent annotation的对象，验证申请内存大于64k的情况是否正常，验证是否经过RC策略以及是否为堆内存
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 使用注解的对象申请内存大于64K，验证是否经过RC策略以及是否为堆内存
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RCPermanentLimit2.java jniTestHelper.cpp
 *- @ExecuteClass: RCPermanentLimit2
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */

import java.util.ArrayList;

import com.huawei.ark.annotation.Permanent;


public class RCPermanentLimit2 {
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
    验证加了注解Permanent，申请的内存为64K+1byte的情况
     */
    static void method1(Object obj) {
        try {
            obj = new @Permanent ArrayList<Byte>();

            for (int i = 0; i < 64 * 1024+1; i++) {
                byte tmp = 2;
                ((ArrayList) obj).add(tmp);
            }
//            int check_size = ((ArrayList) obj).size();
//            System.out.println(check_size);
            boolean result1 = checkRC(obj);
            boolean result2 =isHeapObject(obj);
            if (result1 == false && result2 == true) {
                checkNum++;
            } else {
                System.out.println("result1:" + result1);
                System.out.println("isHeapObject:" + result2);
                System.out.println("error in new int test1;in method1");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /*
    验证加了注解Permanent，申请的内存为128K的情况
     */
    static void method2(Object obj) {
        try {
            obj = new @Permanent ArrayList<Byte>();

            for (int i = 0; i < 128 * 1024; i++) {
                byte tmp = 2;
                ((ArrayList) obj).add(tmp);
            }
//            int check_size = ((ArrayList) obj).size();
//            System.out.println(check_size);
            boolean result1 = checkRC(obj);
            boolean result2 =isHeapObject(obj);
            if (result1 == false && result2 == true) {
                checkNum++;
            } else {
                System.out.println("result1:" + result1);
                System.out.println("isHeapObject:" + result2);
                System.out.println("error in new int test1;in method1");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {

        Object obj = null;
        method1(obj);
        Runtime.getRuntime().gc();
        method1(obj);

        method2(obj);
        Runtime.getRuntime().gc();
        method2(obj);
        
        if (checkNum == 4) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("checkNum:" + checkNum);
        }
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
