/*
 *- @TestCaseID:PlacementRCTest03.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:检测在跳转语句（break, continue, return）控制流中，生命周期结束的对象，应该会被立即回收
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1:函数1做在continue ； continue tag的跳转for循环里，检查局部变量的及时回收情况
 * -#step2:函数2做在return的跳转for循环里，检查局部变量的及时回收情况
 * -#step3:函数3做在break的跳转do-while循环里，检查局部变量的及时回收情况
 *- @Expect:a10\na2_i91_j4\nExpect Result\n
 *- @Priority: High
 *- @Source: PlacementRCTest03.java
 *- @ExecuteClass: PlacementRCTest03
 *- @ExecuteArgs:
 *- @Remark:
 *
 *
 */


class PlacementRC_A3 {
    public int count = 0;
    public String className = "temp";

    public PlacementRC_A3(String name) {
        this.className = name;
    }

    public void changeName(String name) {
        this.className = name;
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        synchronized (PlacementRCTest03.lock) {
            if (this.count % 25 == 0) {
                PlacementRCTest03.result += ("End" + count);
            }
        }
    }    

}

public class PlacementRCTest03 {
    public static String result = new String();
    public static String lock = "";
    private volatile static int count = 0;
    private PlacementRC_A3 defInsideUseOutside = null;

    //def outside, only use inside loop
    public static void onlyUseInsideLoop() {
        PlacementRC_A3 a1 = new PlacementRC_A3("a1");
        continueTag:
        for (count = 0; count < 100; count++) {
            a1.changeName("a" + count);
            a1.count = count;
            if (count % 95 == 0) {
                count = 100;
                continue continueTag;
            }
            if (count % 4 == 0) {
                if (count % 8 == 0)
                    continue;
                a1.changeName("a" + 100);
                a1 = new PlacementRC_A3("a10");
                a1.toString();
            }
            if (count >= 95)
                System.out.println("Wrong Result");
        }
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest03.lock) {
            result += "Method1";
        }
    }

    public static PlacementRC_A3 defAndUseInsideLoop() {
        for (count = 0; count < 100; count++) {
            PlacementRC_A3 a2 = new PlacementRC_A3("a2_i" + count);
            a2.count = count;
            a2.changeName("null");
            for (int j = 0; j < 2; j++)
                a2 = new PlacementRC_A3("a2_i" + count + "_j" + j);
            if (count % 4 == 0) {
                a2.changeName("a" + 100);
                a2 = new PlacementRC_A3("a10");
                if (count == 96) {
                    Runtime.getRuntime().runFinalization();
                    synchronized (PlacementRCTest03.lock) {
                        result += "Method2";
                    }
                    return a2;
                }
                a2.toString();
            } else {
                a2.toString();

            }
        }
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest03.lock) {
            result += "Method2";
        }
        return new PlacementRC_A3("wrong");
    }

    public static void main(String[] args) {
        onlyUseInsideLoop();
        System.out.println(defAndUseInsideLoop().className);
        new PlacementRCTest03().defInsideAndUseOutsideLoop();
        //System.out.println(result);
        if(result.contains("Method1") && result.contains("Method2") && result.contains("Method3") && result.contains("End"))
            System.out.println("Expect Result");
        else
            System.out.println("Error Result");

    }

    public void defInsideAndUseOutsideLoop() {
        count = 0;
        breakTag:
        do {
            int choice = count % 4;
            this.defInsideUseOutside = new PlacementRC_A3("a2_i" + count);
            switch (choice) {
                case 1:
                    for (int j = 0; j < 5; j++) {
                        this.defInsideUseOutside = new PlacementRC_A3("a2_i" + count + "_j" + j);
                        this.defInsideUseOutside.count = count;
                    }
                    if (count == 99)
                        this.defInsideUseOutside.toString();
                    break;
                case 2:
                    for (int j = 0; j < 2; j++) {
                        this.defInsideUseOutside.changeName("a2_i" + count + "_j" + j);
                        this.defInsideUseOutside.count = count;
                    }
                    break;
                case 3:
                    for (int j = 0; j < 5; j++) {
                        this.defInsideUseOutside = new PlacementRC_A3("a2_i" + count + "_j" + j);
                        this.defInsideUseOutside.count = count;
                    }
                    if (this.defInsideUseOutside.className.equals("a2_i91_j4")) {
                        System.out.println(this.defInsideUseOutside.className);
                        break breakTag;
                    }
                    if (this.defInsideUseOutside.className.equals("a2_i99_j4")) {
                        System.out.println(this.defInsideUseOutside.className);
                        break breakTag;
                    }

                    break;
                default:
                    this.defInsideUseOutside = new PlacementRC_A3("a2_i" + count);
                    this.defInsideUseOutside.count = count;
            }
            count++;
        } while (this.defInsideUseOutside.count < 100 && count < 100);
        this.defInsideUseOutside.changeName("finish");
        this.defInsideUseOutside = null;
        Runtime.getRuntime().runFinalization();
        synchronized (PlacementRCTest03.lock) {
            result += "Method3";
        }
    }
}


// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full a10\na2_i91_j4\nExpect Result\n
