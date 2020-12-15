/*
 *- @TestCaseID: Maple_MemoryManagement2.0_CB_Thread_01
 *- @TestCaseName: CB_Thread_01
 *- @TestCaseType: Function Testing for placementRCTest
 *- @RequirementName: 运行时支持GCOnly
 *- @Condition:no
 *  -#c1: MultiThreadMode:Cycle is Cycle_a_0038.java;Nocycle is Nocycle_00180.java
 *- @Brief:一组有环的对象，他们的field指向一个hashMap，里面装满了对象
 *  -#step1: 创建一个环，环的类型参考了Cycle_a_0038.java;
 *  -#step2: 两个域test1和test2,里面装满了对象，对象格式参考Nocycle_00180.java，这样就构造了一个大对象。
 *  -#step3: 验证结果正确，再用GCverify验证无内存泄漏。
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: CB_Thread_01.java
 *- @ExecuteClass: CB_Thread_01
 *- @ExecuteArgs:
 */

import java.util.HashMap;

class CB_Nocycle_a_00180_A1 {
    CB_Nocycle_a_00180_B1 b1_0;
    CB_Nocycle_a_00180_D1 d1_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_A1(String strObjectName) {
        b1_0 = null;
        d1_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + b1_0.a + d1_0.a;
    }
}

class CB_Nocycle_a_00180_A2 {
    CB_Nocycle_a_00180_B2 b2_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_A2(String strObjectName) {
        b2_0 = null;
        a = 102;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + b2_0.a;
    }
}

class CB_Nocycle_a_00180_A3 {
    CB_Nocycle_a_00180_B2 b2_0;
    CB_Nocycle_a_00180_C2 c2_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_A3(String strObjectName) {
        b2_0 = null;
        c2_0 = null;
        a = 103;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + b2_0.a + c2_0.a;
    }
}

class CB_Nocycle_a_00180_A4 {
    CB_Nocycle_a_00180_B3 b3_0;
    CB_Nocycle_a_00180_C2 c2_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_A4(String strObjectName) {
        b3_0 = null;
        c2_0 = null;
        a = 104;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + b3_0.a + c2_0.a;
    }
}

class CB_Nocycle_a_00180_B1 {
    CB_Nocycle_a_00180_D2 d2_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_B1(String strObjectName) {
        d2_0 = null;
        a = 201;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + d2_0.a;
    }
}

class CB_Nocycle_a_00180_B2 {
    CB_Nocycle_a_00180_C1 c1_0;
    CB_Nocycle_a_00180_D1 d1_0;
    CB_Nocycle_a_00180_D2 d2_0;
    CB_Nocycle_a_00180_D3 d3_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_B2(String strObjectName) {
        c1_0 = null;
        d1_0 = null;
        d2_0 = null;
        d3_0 = null;
        a = 202;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + c1_0.a + d1_0.a + d2_0.a + d3_0.a;
    }
}

class CB_Nocycle_a_00180_B3 {
    CB_Nocycle_a_00180_C1 c1_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_B3(String strObjectName) {
        c1_0 = null;
        a = 203;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + c1_0.a;
    }
}

class CB_Nocycle_a_00180_C1 {
    CB_Nocycle_a_00180_D1 d1_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_C1(String strObjectName) {
        d1_0 = null;
        a = 301;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + d1_0.a;
    }
}

class CB_Nocycle_a_00180_C2 {
    CB_Nocycle_a_00180_D2 d2_0;
    CB_Nocycle_a_00180_D3 d3_0;
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_C2(String strObjectName) {
        d2_0 = null;
        d3_0 = null;
        a = 302;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + d2_0.a + d3_0.a;
    }
}

class CB_Nocycle_a_00180_D1 {
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_D1(String strObjectName) {
        a = 401;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + a;
    }
}

class CB_Nocycle_a_00180_D2 {
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_D2(String strObjectName) {
        a = 402;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + a;
    }
}

// 该树有12个节点
class CB_Nocycle_a_00180_D3 {
    int a;
    int sum;
    String strObjectName;

    CB_Nocycle_a_00180_D3(String strObjectName) {
        a = 403;
        sum = 0;
        this.strObjectName = strObjectName;
        // System.out.println("RC-Testing_Construction_D3_"+strObjectName);
    }

    void add() {
        sum = a + a;
    }
}

class CB_Nocycle_a_00180 {
    public CB_Nocycle_a_00180() {
        CB_Nocycle_a_00180_A1 a1_main = new CB_Nocycle_a_00180_A1("a1_main");
        CB_Nocycle_a_00180_A2 a2_main = new CB_Nocycle_a_00180_A2("a2_main");
        CB_Nocycle_a_00180_A3 a3_main = new CB_Nocycle_a_00180_A3("a3_main");
        CB_Nocycle_a_00180_A4 a4_main = new CB_Nocycle_a_00180_A4("a4_main");
        a1_main.b1_0 = new CB_Nocycle_a_00180_B1("b1_0");
        a1_main.d1_0 = new CB_Nocycle_a_00180_D1("d1_0");
        a1_main.b1_0.d2_0 = new CB_Nocycle_a_00180_D2("d2_0");
        a2_main.b2_0 = new CB_Nocycle_a_00180_B2("b2_0");
        a2_main.b2_0.c1_0 = new CB_Nocycle_a_00180_C1("c1_0");
        a2_main.b2_0.d1_0 = new CB_Nocycle_a_00180_D1("d1_0");
        a2_main.b2_0.d2_0 = new CB_Nocycle_a_00180_D2("d2_0");
        a2_main.b2_0.d3_0 = new CB_Nocycle_a_00180_D3("d3_0");
        a2_main.b2_0.c1_0.d1_0 = new CB_Nocycle_a_00180_D1("d1_0");
        a3_main.b2_0 = new CB_Nocycle_a_00180_B2("b2_0");
        a3_main.b2_0.c1_0 = new CB_Nocycle_a_00180_C1("c1_0");
        a3_main.b2_0.c1_0.d1_0 = new CB_Nocycle_a_00180_D1("d1_0");
        a3_main.b2_0.d1_0 = new CB_Nocycle_a_00180_D1("d1_0");
        a3_main.b2_0.d2_0 = new CB_Nocycle_a_00180_D2("d2_0");
        a3_main.b2_0.d3_0 = new CB_Nocycle_a_00180_D3("d3_0");
        a3_main.c2_0 = new CB_Nocycle_a_00180_C2("c2_0");
        a3_main.c2_0.d2_0 = new CB_Nocycle_a_00180_D2("d2_0");
        a3_main.c2_0.d3_0 = new CB_Nocycle_a_00180_D3("d3_0");

        a4_main.b3_0 = new CB_Nocycle_a_00180_B3("b3_0");
        a4_main.b3_0.c1_0 = new CB_Nocycle_a_00180_C1("c1_0");
        a4_main.b3_0.c1_0.d1_0 = new CB_Nocycle_a_00180_D1("d1_0");
        a4_main.c2_0 = new CB_Nocycle_a_00180_C2("c2_0");
        a4_main.c2_0.d2_0 = new CB_Nocycle_a_00180_D2("d2_0");
        a4_main.c2_0.d3_0 = new CB_Nocycle_a_00180_D3("d3_0");
    }
}

class CB_Thread_01_A1 {
    static HashMap test1;
    static
    int a;
    CB_Thread_01_A2 a2_0;
    CB_Thread_01_A3 a3_0;
    int sum;
    String strObjectName;

    CB_Thread_01_A1(String strObjectName) {
        a2_0 = null;
        a3_0 = null;
        a = 101;
        sum = 0;
        this.strObjectName = strObjectName;
        // System.out.println("RC-Testing_Construction_A1_"+strObjectName);
    }

    void add() {
        sum = a + a2_0.a + a3_0.a;
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        // System.out.println("A finalize");

        CB_Thread_01.check = this;
    }
}

class CB_Thread_01_A2 {
    volatile static HashMap test2;
    CB_Thread_01_A1 a1_0;
    CB_Thread_01_A3 a3_0;
    int a;
    int sum;
    String strObjectName;

    CB_Thread_01_A2(String strObjectName) {
        a1_0 = null;
        a3_0 = null;
        a = 102;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + CB_Thread_01_A1.a + a3_0.a;
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
//        System.out.println("B finalize");
    }
}

class CB_Thread_01_A3 {
    static CB_Nocycle_a_00180[] test1;
    CB_Thread_01_A1 a1_0;
    CB_Thread_01_A2 a2_0;
    int a;
    int sum;
    String strObjectName;

    CB_Thread_01_A3(String strObjectName) {
        a1_0 = null;
        a2_0 = null;
        a = 103;
        sum = 0;
        this.strObjectName = strObjectName;
    }

    void add() {
        sum = a + CB_Thread_01_A1.a + a2_0.a;
    }
}

class CB_Thread_01_test extends Thread {
    private static CB_Thread_01_A1 a1_main = null;

    private static void test_CB_Thread_01(int times) {
        CB_Thread_01_A1.test1 = new HashMap();
        CB_Thread_01_A2.test2 = new HashMap();
        for (int i = 0; i < times; i++) {
            CB_Thread_01_A1.test1.put(i, new CB_Nocycle_a_00180());
            CB_Thread_01_A2.test2.put(i, new CB_Nocycle_a_00180());
        }
    }

    public void run() {
        a1_main = new CB_Thread_01_A1("a1_main");
        a1_main.a2_0 = new CB_Thread_01_A2("a2_0");
        a1_main.a2_0.a1_0 = a1_main;
        a1_main.a2_0.a3_0 = new CB_Thread_01_A3("a3_0");
        a1_main.a3_0 = a1_main.a2_0.a3_0;
        a1_main.a3_0.a1_0 = a1_main;
        a1_main.a3_0.a2_0 = a1_main.a2_0;
        test_CB_Thread_01(10000);
        a1_main = null;
        CB_Thread_01.check = null;
    }
}

public class CB_Thread_01 {
    public static CB_Thread_01_A1 check = null;
    private static CB_Thread_01_A1 a1_main = null;
    private static CB_Thread_01_test cbtest1;
    private volatile static CB_Thread_01_test cbtest2;

    public static void main(String[] args) {
        a1_main = new CB_Thread_01_A1("a1_main");
        a1_main.a2_0 = new CB_Thread_01_A2("a2_0");
        a1_main.a2_0.a1_0 = a1_main;
        a1_main.a2_0.a3_0 = new CB_Thread_01_A3("a3_0");
        a1_main.a3_0 = a1_main.a2_0.a3_0;
        a1_main.a3_0.a1_0 = a1_main;
        a1_main.a3_0.a2_0 = a1_main.a2_0;
        a1_main.add();
        a1_main.a2_0.add();
        a1_main.a2_0.a3_0.add();
        a1_main = null;
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();

        try {
            int result = CB_Thread_01.check.sum + CB_Thread_01.check.a2_0.sum + CB_Thread_01.check.a2_0.a3_0.sum;
            if (result == 918) {
                System.out.println("ExpectResult");
            }
        } catch (NullPointerException n) {
            System.out.println("ErrorResult");
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
