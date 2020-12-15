/*
 *- @TestCaseID:maple/runtime/rc/annotation/Permanent/RCPermanentThread
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:多线程下调用PermanentTest的case，验证是否经过RC策略以及是否为堆内存
 *- @Condition:
 * -#c1:
 *- @Brief:functionTest
 * -#step1:
 * 多线程下调用PermanentTest的case,包括caseRC0411,RC0412,RC0413,RC0414,RC0415,验证是否经过RC策略以及是否为堆内存
 *- @Expect:ExpectResult\nExpectResult\n
 *- @Priority: High
 *- @Source: RCPermanentThread.java jniTestHelper.cpp
 *- @ExecuteClass: RCPermanentThread
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:liuweiqing l00481345
 */
import java.util.ArrayList;

import com.huawei.ark.annotation.Permanent;
import sun.misc.Cleaner;

import java.lang.ref.*;

public class RCPermanentThread {
    public static void main(String[] args) throws InterruptedException {
        rc_testcase_main_wrapper();
    }

    private static void rc_testcase_main_wrapper() throws InterruptedException {
        RCPermanentTest rcPermanentTest = new RCPermanentTest();
        RCPermanentTest2 rcPermanentTest2 = new RCPermanentTest2();
        RCPermanentTest3 rcPermanentTest3 = new RCPermanentTest3();
        RCPermanentTest4 rcPermanentTest4 = new RCPermanentTest4();
        RCPermanentTest5 rcPermanentTest5 = new RCPermanentTest5();

        rcPermanentTest.start();
        rcPermanentTest2.start();
        rcPermanentTest3.start();
        rcPermanentTest4.start();
        rcPermanentTest5.start();

        rcPermanentTest.join();
        rcPermanentTest2.join();
        rcPermanentTest3.join();
        rcPermanentTest4.join();
        rcPermanentTest5.join();

        if (rcPermanentTest.check() && rcPermanentTest2.check() && rcPermanentTest3.check() && rcPermanentTest4.check() && rcPermanentTest5.check()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("rcPermanentTest.check() :" + rcPermanentTest.check());
            System.out.println("rcPermanentTest2.check() :" + rcPermanentTest2.check());
            System.out.println("rcPermanentTest3.check() :" + rcPermanentTest3.check());
            System.out.println("rcPermanentTest4.check() :" + rcPermanentTest4.check());
            System.out.println("rcPermanentTest5.check() :" + rcPermanentTest5.check());
        }
    }
}


class RCPermanentTest extends Thread {
    public boolean checkout;

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

    /*
    验证new int [8]
     */
    static void method2(Object obj) {
        obj = new int[8];
        boolean result1 = checkRC(obj);
        if (result1 == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in new int [8];in method2");
        }
    }

    /*
    验证new @Permanent int[8]
    */
    static void method3(Object obj) {
        obj = new @Permanent int[1];
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new @Permanent int[8];in method3");
        }
    }

    /*
    验证new Integer @Permanent [10]
    */
    static void method4(Object obj) {
        Integer[] arr = new Integer@Permanent[10];
        boolean result1 = checkRC(obj);
        noleak.add(arr); // let leak check happy
        arr[0] = new Integer(10000);
        arr[9] = new Integer(20000);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new Integer @Permanent [10];in method4");
        }
    }

    /*
   验证new @Permanent Integer  [10]
   */
    static void method5(Object obj) {
        Integer[] arr = new @Permanent Integer[10];
        boolean result1 = checkRC(obj);
        noleak.add(arr); // let leak check happy
        arr[0] = new Integer(10000);
        arr[9] = new Integer(20000);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new @Permanent Integer [10];in method5");
        }
    }

    /*
    验证new Integer[10];
    */
    static void method6(Object obj) {
        obj = new Integer[10];
        boolean result1 = checkRC(obj);

        if (result1 == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in new Integer[10];in method6");
        }
    }

    /*
    验证new @Permanent ArrayList[]{};
    */
    static void method7(Object obj) {
        obj = new @Permanent ArrayList[]{};
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new @Permanent ArrayList[]{};in method7");
        }
    }

    /*
    验证new ArrayList @Permanent []{};
    */
    static void method8(Object obj) {
        obj = new ArrayList@Permanent[]{};
        boolean result1 = checkRC(obj);
        if (result1 == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in new ArrayList @Permanent []{};in method8");
        }
    }

    /*
    验证new ArrayList []{};
    */
    static void method9(Object obj) {
        obj = new ArrayList[]{};
        boolean result1 = checkRC(obj);
        if (result1 == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in new ArrayList []{}; method9");
        }
    }


    static ArrayList<Object> noleak = new ArrayList<>();

    public void run(
        ) {
        Object obj = null;
        method1(obj);
        method2(obj);
        method3(obj);
        method4(obj);
        method5(obj);
        method6(obj);
        method7(obj);
        method8(obj);
        method9(obj);
  
        if (checkNum == 9) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println(checkNum);
        }
    }

    public boolean check() {
        return checkout;
    }
}

class RCPermanentTest2 extends Thread {
    public boolean checkout;

    public static native boolean isHeapObject(Object obj);

    public static native int refCount(Object obj);

    public static int checkNum = 0;

    static Object owner;

    public static Integer INT = new @Permanent Integer(100);
    public static Integer INT2 = new Integer(100);
    static String STRING = new @Permanent String("test");
    static Long LONG = new @Permanent Long(23534);
    static Long LONG2 = null;

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
    }

    /*
    验证public final static Integer INT = new @Permanent Integer(100);
    */
    static void method1() {
        boolean result = checkRC(INT);
        boolean heapResult = isHeapObject(INT);

        if ((result == false) && (heapResult == false)) {
            checkNum++;
        } else {
            System.out.println("error in method1");
            System.out.println("result:" + result + "    isHeap:" + heapResult);
            System.out.println(String.valueOf(result));
            System.out.println(String.valueOf(heapResult));
        }
    }

    /*
    验证public final static Integer INT2 = new Integer(100);
    */
    static void method2() {
        boolean result = checkRC(INT2);
        if (result == true && isHeapObject(INT2) == true) {
            checkNum++;
        } else {
            System.out.println("error in method2");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(INT2));
        }
    }

    /*
    验证static String STRING = new @Permanent String("test");
    */
    static void method3() {
        // STRING = new @Permanent (454555);
        boolean result = checkRC(STRING);
        if (result == true && isHeapObject(STRING) == true) {
            checkNum++;
        } else {
            System.out.println("error in method3");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(STRING));
        }
    }

    /*
    验证static Long LONG2 = new  Long("test4");
    */
    static void method4() {
        LONG2 = new Long(2222222);
        boolean result = checkRC(LONG2);
        if (result == true && isHeapObject(LONG2) == true) {
            checkNum++;
        } else {
            System.out.println("error in method4");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(LONG2));
        }
    }

    /*
    验证String obj = new @Permanent Long("test5");
    */
    static void method5() {
        //String obj = new @Permanent String("test5");
        Double obj = new @Permanent Double(3333.3);
        boolean result = checkRC(obj);
        if (result == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in method5");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(obj));
        }
    }

    /*
    验证String obj = new String("test6");
    */
    static void method6() {
        Double obj = new Double(3333.3);
        boolean result = checkRC(obj);
        if (result == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in method6");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(obj));
        }
    }

    /*
    验证obj =  new @Permanent Integer("test");
    */
    static volatile Integer obj = null;

    static void method7() {
        obj = new @Permanent Integer(123);
        boolean result = checkRC(obj);
        if (result == false && isHeapObject(obj) == false) {
            checkNum++;
        } else {
            System.out.println("error in  method7");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(obj));
        }
    }

    /*
    验证obj =  new @Permanent Integer("test");
    */
    static void method8() {
        obj = new Integer(123);
        boolean result = checkRC(obj);
        if (result == true && isHeapObject(obj) == true) {
            checkNum++;
        } else {
            System.out.println("error in method8");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(obj));
        }
    }

    /*
    验证static long LONG = new @Permanent long(23534);
    */
    static void method9() {
        boolean result = checkRC(LONG);
        if (result == false && isHeapObject(LONG) == false) {
            checkNum++;
        } else {
            System.out.println("error in method9");
            System.out.println("result:" + result + "    isHeap:" + isHeapObject(LONG));
        }
    }


    public void run() {
        method1();
        method2();
        method3();
        method4();
        method5();
        method6();
        method7();
        method8();
        method9();
        //System.out.println(checkNum);
        if (checkNum == 9) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println(checkNum);
        }
    }

    public boolean check() {
        return checkout;
    }
}

class RCPermanentTest3 extends Thread {
    public boolean checkout;

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


    public void run() {

        method1();
        Runtime.getRuntime().gc();
        method1();

        method2();
        Runtime.getRuntime().gc();
        method2();
        
        if (checkNum == 4) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println(checkNum);
        }
    }

    public boolean check() {
        return checkout;
    }
}

class RCPermanentTest4 extends Thread {
    public boolean checkout;

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
    }

    /*
      验证环场景
       */
    static void method1() {
        Cycle_A a = new Cycle_A();
        a.b = null;
        a.b = new @Permanent Cycle_B();
        a.b.a = a;
        int result = a.add() + a.b.add();
        if (result == 16) {
            checkNum++;
        } else {
            System.out.println("result:" + result);
        }
        boolean resultA1 = checkRC(a);
        boolean resultA2 = isHeapObject(a);
        boolean resultB1 = checkRC(a.b);
        boolean resultB2 = isHeapObject(a.b);
        if (String.valueOf(resultA1).equals("true") && String.valueOf(resultA2).equals("true") && String.valueOf(resultB1).equals("false") && String.valueOf(resultB2).equals("true")) {
            //if (result1 == false && result2 == true) {
            checkNum++;
        } else {
            System.out.println("error");
            System.out.println("CycleA_checkRC:" + resultA1);
            System.out.println("CycleA_isHeap:" + resultA2);
            System.out.println("CycleB_checkRC:" + resultB1);
            System.out.println("CycleB_isHeap:" + resultB2);
            //System.out.println("result1:" + result1 + "       is Heap:" + result2);
        }

    }

    public void run() {

        method1();
       
        if (checkNum == 2) {
            checkout = true;
        } else {
            System.out.println(checkNum);
            checkout = false;

        }
    }

    public boolean check() {
        return checkout;
    }
}


class Cycle_A {
    Cycle_B b;
    static int sum;
    static int a;

    Cycle_A() {
        a = 3;
    }

    int add() {
        sum = a + b.b;
        return sum;
    }

}

class Cycle_B {
    Cycle_A a;
    static int sum;
    static int b;

    Cycle_B() {
        b = 5;

    }

    int add() {
        sum = a.a + b;
        return sum;
    }

}


class RCPermanentTest5 extends Thread {
    public boolean checkout;

    public static native boolean isHeapObject(Object obj);

    public static native int refCount(Object obj);

    static Object owner;
    static Object referent = new Object();
    static int checkSum;

    public void run() {
        Object obj = null;
        methodRef1(obj);
        methodRef2(obj);
        methodRef3(obj);
        methodFinal(obj);
        
        if (checkSum == 4) {
            checkout = true;
        } else {
            checkout = false;
            System.out.println(checkSum);
        }
    }

    public boolean check() {
        return checkout;
    }

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
    }


    static void methodRef1(Object obj) {
        obj = new @Permanent WeakReference(referent);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }

    static void methodRef2(Object obj) {
        obj = new @Permanent SoftReference(referent);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }

    static void methodRef3(Object obj) {
        ReferenceQueue rq = new ReferenceQueue();
        obj = new @Permanent PhantomReference(referent, rq);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }

    static void methodFinal(Object obj) {
        obj = new @Permanent FinalizableObject();
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }


}

class FinalizableObject {
    // static int value;

    public void finalize() {
        System.out.println("ExpectResult");
    }
}
// DEPENDENCE: jniTestHelper.cpp
// EXEC:%maple  %f jniTestHelper.cpp %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
