/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
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
