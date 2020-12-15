/*
 * -@TestCaseID:maple/runtime/rc/function/RC_Thread02/RC_Thread_03.java
 * -@TestCaseName:RC_Thread_03
 * -@RequirementName:[运行时需求]支持自动内存管理
 * -@Title/Destination:Multi Thread reads or writes static para.mofidfy from Nocycle_a_00180
 * -@Condition: no
 * -#c1
 * -@Brief:Multi Thread reads or writes static para. It is modified from Nocycle_a_00180. At the same time，another thread is GC.
 * -#step1
 * -@Expect:ExpectResult\nExpectResult\n
 * -@Priority: High
 * -@Source: RC_Thread_03.java
 * -@ExecuteClass: RC_Thread_03
 * -@ExecuteArgs:
 * -@Remark:
 *
 */

class RcThread0301 extends Thread {
    public void run() {
        RC_Thread_03 rcth01 = new RC_Thread_03();
        try {
            rcth01.setA1null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0302 extends Thread {
    public void run() {
        RC_Thread_03 rcth01 = new RC_Thread_03();
        try {
            rcth01.setA2null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0303 extends Thread {
    public void run() {
        RC_Thread_03 rcth01 = new RC_Thread_03();
        try {
            rcth01.setA3();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0304 extends Thread {
    public void run() {
        RC_Thread_03 rcth01 = new RC_Thread_03();
        try {
            rcth01.setA3null();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread0305 extends Thread {
    public void run() {
        RC_Thread_03 rcth01 = new RC_Thread_03();
        try {
            rcth01.setA4();
        } catch (NullPointerException e) {
            // do nothing
        }
    }
}

class RcThread03GC extends Thread {
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

public class RC_Thread_03 {
    private static volatile RcThread03A1 a1_main = null;
    private static volatile RcThread03A2 a2_main = null;
    private static volatile RcThread03A3 a3_main = null;
    private static volatile RcThread03A4 a4_main = null;

    RC_Thread_03() {
        try {
            a1_main = new RcThread03A1("a1_main");
            a2_main = new RcThread03A2("a2_main");
            a3_main = new RcThread03A3("a3_main");
            a4_main = new RcThread03A4("a4_main");
            a1_main.b1_0 = new RcThread03B1("b1_0");
            a1_main.d1_0 = new RcThread03D1("d1_0");
            a1_main.b1_0.d2_0 = new RcThread03D2("d2_0");
            a2_main.b2_0 = new RcThread03B2("b2_0");
            a2_main.b2_0.c1_0 = new RcThread03C1("c1_0");
            a2_main.b2_0.d1_0 = new RcThread03D1("d1_0");
            a2_main.b2_0.d2_0 = new RcThread03D2("d2_0");
            a2_main.b2_0.d3_0 = new RcThread03D3("d3_0");
            a2_main.b2_0.c1_0.d1_0 = new RcThread03D1("d1_0");
            a3_main.b2_0 = new RcThread03B2("b2_0");
            a3_main.b2_0.c1_0 = new RcThread03C1("c1_0");
            a3_main.b2_0.c1_0.d1_0 = new RcThread03D1("d1_0");
            a3_main.b2_0.d1_0 = new RcThread03D1("d1_0");
            a3_main.b2_0.d2_0 = new RcThread03D2("d2_0");
            a3_main.b2_0.d3_0 = new RcThread03D3("d3_0");
            a3_main.c2_0 = new RcThread03C2("c2_0");
            a3_main.c2_0.d2_0 = new RcThread03D2("d2_0");
            a3_main.c2_0.d3_0 = new RcThread03D3("d3_0");
            a4_main.b3_0 = new RcThread03B3("b3_0");
            a4_main.b3_0.c1_0 = new RcThread03C1("c1_0");
            a4_main.b3_0.c1_0.d1_0 = new RcThread03D1("d1_0");
            a4_main.c2_0 = new RcThread03C2("c2_0");
            a4_main.c2_0.d2_0 = new RcThread03D2("d2_0");
            a4_main.c2_0.d3_0 = new RcThread03D3("d3_0");
        } catch (NullPointerException e) {
            // do nothing
        }
    }

    public static void main(String[] args) {
        RcThread03GC gc = new RcThread03GC();
        gc.run();
        for(int start = 0; start < 10; start++) {
            rcTestcaseMainWrapper();
        }
        System.out.println("ExpectResult");

    }

    private static void rcTestcaseMainWrapper() {
        RcThread0301 t1 = new RcThread0301();
        RcThread0302 t2 = new RcThread0302();
        RcThread0303 t3 = new RcThread0303();
        RcThread0304 t4 = new RcThread0304();
        RcThread0305 t5 = new RcThread0305();
        t1.start();
        t2.start();
        t3.start();
        t4.start();
        t5.start();
        try {
            t1.join();
            t2.join();
            t3.join();
            t4.join();
            t5.join();

        }catch(InterruptedException e){
            // do nothing
        }
    }

    void setA1null() {
        a1_main = null;
    }

    void setA2null() {
        a2_main = null;
    }

    void setA3() {
        try {
            a3_main.c2_0.d2_0 = new RcThread03D2("new");
            a3_main = new RcThread03A3("a3_new");
            a3_main = null;
        } catch (NullPointerException e) {
            // do nothing
        }
    }

    void setA3null() {
        a3_main = null;
    }

    void setA4() {
        try {
            a4_main = new RcThread03A4("a4_new");
        } catch (NullPointerException e) {
            // do nothing
        }
    }

    class RcThread03A1 {
        volatile RcThread03B1 b1_0;
        volatile RcThread03D1 d1_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03A1(String strObjectName) {
            b1_0 = null;
            d1_0 = null;
            a = 101;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03A2 {
        volatile RcThread03B2 b2_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03A2(String strObjectName) {
            b2_0 = null;
            a = 102;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03A3 {
        volatile RcThread03B2 b2_0;
        volatile RcThread03C2 c2_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03A3(String strObjectName) {
            b2_0 = null;
            c2_0 = null;
            a = 103;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03A4 {
        volatile RcThread03B3 b3_0;
        volatile RcThread03C2 c2_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03A4(String strObjectName) {
            b3_0 = null;
            c2_0 = null;
            a = 104;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03B1 {
        volatile RcThread03D2 d2_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03B1(String strObjectName) {
            d2_0 = null;
            a = 201;
            sum = 0;
            this.strObjectName = strObjectName;
        }

    }

    class RcThread03B2 {
        volatile RcThread03C1 c1_0;
        volatile RcThread03D1 d1_0;
        volatile RcThread03D2 d2_0;
        volatile RcThread03D3 d3_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03B2(String strObjectName) {
            c1_0 = null;
            d1_0 = null;
            d2_0 = null;
            d3_0 = null;
            a = 202;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03B3 {
        volatile RcThread03C1 c1_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03B3(String strObjectName) {
            c1_0 = null;
            a = 203;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03C1 {
        volatile RcThread03D1 d1_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03C1(String strObjectName) {
            d1_0 = null;
            a = 301;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03C2 {
        volatile RcThread03D2 d2_0;
        volatile RcThread03D3 d3_0;
        int a;
        int sum;
        String strObjectName;

        RcThread03C2(String strObjectName) {
            d2_0 = null;
            d3_0 = null;
            a = 302;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03D1 {
        int a;
        int sum;
        String strObjectName;

        RcThread03D1(String strObjectName) {
            a = 401;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03D2 {
        int a;
        int sum;
        String strObjectName;

        RcThread03D2(String strObjectName) {
            a = 402;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }

    class RcThread03D3 {
        int a;
        int sum;
        String strObjectName;

        RcThread03D3(String strObjectName) {
            a = 403;
            sum = 0;
            this.strObjectName = strObjectName;
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\nExpectResult\n
