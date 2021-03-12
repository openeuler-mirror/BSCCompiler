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
