/*
 * -@TestCaseID:maple/runtime/rc/function/RC_Thread02/RC_Thread_05.java
 * -@TestCaseName:RC_Thread_05
 * -@RequirementName:[运行时需求]支持自动内存管理
 * -@Title/Destination:Multi Thread reads or writes static para.It is modified from Cycle_B_1_00180.At the same time,another thread is doing GC.
 * -@Condition: no
 * -#c1
 * -@Brief:Multi Thread reads or writes static para.It is modified from Cycle_B_1_00180.At the same time,another thread is doing GC.
 * -#step1
 * -@Expect:ExpectResult\nExpectResult\n
 * -@Priority: High
 * -@Source: RC_Thread_05.java
 * -@ExecuteClass: RC_Thread_05
 * -@ExecuteArgs:
 * -@Remark:
 *
 */

class RcThread0501 extends Thread {
    public void run() {
        RC_Thread_05 rcth01 = new RC_Thread_05();
        try {
            rcth01.setA1null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0502 extends Thread {
    public void run() {
        RC_Thread_05 rcth01 = new RC_Thread_05();
        try {
            rcth01.setA4null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0503 extends Thread {
    public void run() {
        RC_Thread_05 rcth01 = new RC_Thread_05();
        try {
            rcth01.setA5null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0504 extends Thread {
    public void run() {
        RC_Thread_05 rcth01 = new RC_Thread_05();
        try {
            rcth01.setA1();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0505 extends Thread {
    public void run() {
        RC_Thread_05 rcth01 = new RC_Thread_05();
        try {
            rcth01.checkA3();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0506 extends Thread {
    public void run() {
        RC_Thread_05 rcth01 = new RC_Thread_05();
        try {
            rcth01.setA3_a(5);
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread05GC extends Thread {
    public void run() {
        int start = 0;
        do {
            Runtime.getRuntime().gc();
            start++;
        } while (start < 50);
        if (start == 50) {
            System.out.println("ExpectResult");
        }
    }
}

public class RC_Thread_05 {
    private static volatile RcThread05A1 a1Main = null;
    private static volatile RcThread05A4 a4Main = null;
    private static volatile RcThread05A5 a5Main = null;

    RC_Thread_05() {
        try {
            a1Main = new RcThread05A1();
            a1Main.a2_0 = new RcThread05A2();
            a1Main.a2_0.a3_0 = new RcThread05A3();
            a4Main = new RcThread05A4();
            a5Main = new RcThread05A5();
            a4Main.a1_0 = a1Main;
            a5Main.a1_0 = a1Main;
            a1Main.a2_0.a3_0.a1_0 = a1Main;
        } catch (NullPointerException e) {
            // do nothing
        }
    }

    public static void main(String[] args) {
        cyclePatternWrapper();
        RcThread05GC gc = new RcThread05GC();
        gc.run();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        rcTestcaseMainWrapper();
        System.out.println("ExpectResult");
    }

    private static void cyclePatternWrapper() {
        a1Main = new RcThread05A1();
        a1Main.a2_0 = new RcThread05A2();
        a1Main.a2_0.a3_0 = new RcThread05A3();
        a4Main = new RcThread05A4();
        a5Main = new RcThread05A5();
        a4Main.a1_0 = a1Main;
        a5Main.a1_0 = a1Main;
        a1Main.a2_0.a3_0.a1_0 = a1Main;
        a1Main = null;
        a4Main = null;
        a5Main = null;
        Runtime.getRuntime().gc();    // 单独构造一次Cycle_B_1_00180环，通过gc学习到环模式
    }

    private static void rcTestcaseMainWrapper() {
        RcThread0501 t1 = new RcThread0501();
        RcThread0502 t2 = new RcThread0502();
        RcThread0503 t3 = new RcThread0503();
        RcThread0504 t4 = new RcThread0504();
        RcThread0505 t5 = new RcThread0505();
        RcThread0506 t6 = new RcThread0506();
        t1.start();
        t2.start();
        t3.start();
        t4.start();
        t5.start();
        t6.start();
        try {
            t1.join();
            t2.join();
            t3.join();
            t4.join();
            t5.join();
            t6.join();
        }catch(InterruptedException e){
            // do nothing
        }
    }

    void setA1null() {
        a1Main = null;
    }

    void setA4null() {
        a4Main = null;
    }

    void setA5null() {
        a5Main = null;
    }

    void setA1() {
        try {
            this.a1Main = new RcThread05A1();
            a1Main.a2_0 = new RcThread05A2();
            a1Main.a2_0.a3_0 = new RcThread05A3();
        } catch (NullPointerException e) {
            // do nothing
        }
    }

    void checkA3() {
        try {
            int a = a1Main.a2_0.a3_0.a;
        } catch (NullPointerException e) {
            // do nothing
        }
    }

    void setA3_a(int a) {
        try {
            this.a1Main.a2_0.a3_0.a = a;
        } catch (NullPointerException e) { }
    }

    static class RcThread05A1 {
        volatile RcThread05A2 a2_0;
        int a;
        int sum;

        RcThread05A1() {
            a2_0 = null;
            a = 1;
            sum = 0;
        }
    }

    static class RcThread05A2 {
        volatile RcThread05A3 a3_0;
        int a;
        int sum;

        RcThread05A2() {
            a3_0 = null;
            a = 2;
            sum = 0;
        }
    }

    static class RcThread05A3 {
        volatile RcThread05A1 a1_0;
        int a;
        int sum;

        RcThread05A3() {
            a1_0 = null;
            a = 3;
            sum = 0;
        }
    }

    static class RcThread05A4 {
        volatile RcThread05A1 a1_0;
        int a;
        int sum;

        RcThread05A4() {
            a1_0 = null;
            a = 4;
            sum = 0;
        }
    }

    static class RcThread05A5 {
        volatile RcThread05A1 a1_0;
        int a;
        int sum;

        RcThread05A5() {
            a1_0 = null;
            a = 5;
            sum = 0;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
