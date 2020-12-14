/*
 *- @TestCaseID:PlacementRCTest01.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:检测在循环语句（for, while, do-while）控制流中，生命周期结束的对象，应该会被立即回收
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest通过重写finalize（）方法对回收的及时性做检测
 * -#step1:函数1测试场景：对象初始化在for循环外，只在for循环内部使用；
 * -#step2:函数2测试场景：对象初始化和使用都在for循环内；
 * -#step3:函数3测试场景：对象初始化在do-while循环内，循环之后，仍有使用；
 * -#step4:函数4测试场景：对象初始化在for循环外，只在for循环内部部分分支（通过if判断）使用；
 * -#step5:函数5测试场景：对象初始化在while循环外，while循环后仍然存活；在while循环内有局部对象的初始化和使用
 * -#step6:判断依据：End字符串和Method的位置关系
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: PlacementRCTest01.java
 *- @ExecuteClass: PlacementRCTest01
 *- @ExecuteArgs:
 *- @Remark:
 *
 *
 */
class B {
    static A a;

    @Override
    public void finalize() throws Throwable {
        super.finalize();
    }
}

class A {
    public int count = 0;
    public String className = "A";

    public A(String name) {
        this.className = name;
    }

    public void changeName(String name) {
        this.className = name;
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        synchronized (PlacementRCTest01.lock) {
            if (this.count % 25 == 0) {
                PlacementRCTest01.result += ("End");
            }
        }
        B.a = this;
    }

}

public class PlacementRCTest01 {
    public volatile static String result = new String();
    public static String lock = "";
    private static volatile int count = 0;
    private static A infiniteLoop = null;
    private A defInsideUseOutside = null;

    public static void onlyUseInsideLoop() {
        A a1 = new A("a1");
        for (count = 0; count < 100; count++) {
            a1.changeName("a" + count);
	    a1.count = count;
            if (count == 99)
                a1.toString();
        }
        Runtime.getRuntime().runFinalization();
		//try{ Thread.sleep(5000); } catch( Exception e ){ }
        synchronized (PlacementRCTest01.lock) {
            result += "Method1";
        }
    }

    public static void defAndUseInsideLoop() {
        for (count = 0; count < 100; count++) {
            A a2 = new A("a2_i" + count);
	    a2.count = count;
            a2.changeName("null");
            for (int j = 0; j < 5; j++) {
                a2 = new A("a2_i" + count + "_j" + j);
            }
            if (count == 99) {
                a2.toString();
            }
        }
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest01.lock) {
            result += "Method2";
        }
    }
	
    public void defInsideAndUseOutsideLoop() {
        count = 0;
        do {
            this.defInsideUseOutside = new A("a2_i" + count);
	    this.defInsideUseOutside.count = count;
            for (int j = 0; j < 2; j++)
                this.defInsideUseOutside = new A("a2_i" + count + "_j" + j);
            if (count == 99)
                this.defInsideUseOutside.toString();
            count++;
        } while (this.defInsideUseOutside.count < 100 && count < 100);
        this.defInsideUseOutside.changeName("finish");
        this.defInsideUseOutside = null;
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest01.lock) {
            result += "Method3";
        }
    }

    public static void defInBranchInsideLoop() {
        A a4 = new A("a4");
        for (count = 0; count < 100; count++) {
            a4.changeName("a" + count);
	    a4.count = count;
            if (count % 2 == 0) {
                a4 = new A("a4_" + count);
            }
            a4.toString();
        }
        a4.changeName("End");
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest01.lock) {
            result += "Method4";
        }
    }

    public static void infiniteLoop() {
        infiniteLoop = new A("infinite loop");
        count = 0;
        while (count < 100) {
            A a5 = new A("a2_i" + count);
            a5.changeName("null");
	    a5.count = count;
            infiniteLoop.changeName("infinite loop" + count);
            for (int j = 0; j < 2; j++) {
                a5 = new A("a2_i" + count + "_j" + j);
            }
            if (count == 99) {
                a5.toString();
            }
            count++;
        }
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest01.lock) {
            result += "Method5";
        }
    }

    public static void main(String[] args) {
        onlyUseInsideLoop();	
        defAndUseInsideLoop();
        new PlacementRCTest01().defInsideAndUseOutsideLoop();
        defInBranchInsideLoop();
        infiniteLoop();
        //System.out.println(result);
        if(result.contains("Method1") && result.contains("Method2") && result.contains("Method3") && result.contains("Method4") && result.contains("Method5") && result.contains("End") && infiniteLoop.className.equals("infinite loop99"))
            System.out.println("ExpectResult");
        else
            System.out.println("ErrorResult");
    }


}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
