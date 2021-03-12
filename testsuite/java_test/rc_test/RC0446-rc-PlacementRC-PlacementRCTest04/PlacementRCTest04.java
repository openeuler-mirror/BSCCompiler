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


import java.util.IllegalFormatException;
class PlacementRC_A4 {
    public int count = 0;
    public String className = "A";
    public PlacementRC_A4(String name) {
        this.className = name;
    }
    public void changeName(String name) {
        this.className = name;
    }
    @Override
    public void finalize() throws Throwable {
        super.finalize();
        synchronized (PlacementRCTest04.lock) {
            PlacementRCTest04.result += "End";
        }
    }
}
public class PlacementRCTest04 {
    public static String result = "";
    public static String lock = "";
    static int res = 99;
    public static void main(String[] argv) {
        run1();
        Runtime.getRuntime().runFinalization();
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // do nothing
        }
        if (result.contains("run1") && result.contains("run4") && result.contains("run5") && result.contains("run3")
                && result.contains("run2") && result.contains("End")) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
    private static int run1() {
        int result = 2;
        PlacementRC_A4 place = new PlacementRC_A4("run4");
        try {
            place.className = "run1";
            synchronized (PlacementRCTest04.lock) {
                PlacementRCTest04.result += "run1";
            }
            result = run2();
        } catch (StringIndexOutOfBoundsException e) {
            res--;
        }
        if (res == 96) {
            result = 0;
            res--;
        }
        return result;
    }
    private static int run2() {
        int result = 2;
        try {
            PlacementRC_A4 place = new PlacementRC_A4("run4");
            result = run3();
            place.changeName("run2");
            synchronized (PlacementRCTest04.lock) {
                PlacementRCTest04.result += "run2";
            }
        } catch (ClassCastException e) {
            res -= 2;
        }
        if (res == 95) {
            result = 0;
        }
        return result;
    }
    private static int run3() {
        int result = 3;
        try {
            result = run4();
            PlacementRC_A4 place = new PlacementRC_A4("run4");
            synchronized (PlacementRCTest04.lock) {
                place.className = "run3";
            }
            PlacementRCTest04.result += place.className;
        } catch (IllegalStateException e) {
            res -= 3;
        }
        if (res == 94) {
            result = 0;
        }
        return result;
    }
    private static int run4() {
        int result = 4;
        try {
            PlacementRC_A4 place = new PlacementRC_A4("run4");
            place.className = "run4";
            synchronized (PlacementRCTest04.lock) {
                PlacementRCTest04.result += "run4";
            }
            result = run5();
        } catch (IllegalFormatException e) {
            res -= 4;
        }
        if (res == 93) {
            result = 0;
        }
        return result;
    }
    private static int run5() {
        int result = 5;
        try {
            result = ExcpCover03330();
        } catch (Exception e) {
            res -= 5;
        }
        PlacementRC_A4 place = new PlacementRC_A4("   ");
        if (res == 92) {
            result = 0;
        }
        place.className = "run5";
        synchronized (PlacementRCTest04.lock) {
            PlacementRCTest04.result += "run5";
        }
        return result;
    }
    private static int ExcpCover03330() {
        int result1 = 4;
        PlacementRC_A4 place4 = new PlacementRC_A4("123#456");
        try {
            Integer.parseInt(place4.className);
        } catch (IndexOutOfBoundsException e) {
            res -= 10;
        } catch (IllegalArgumentException e) {
            res--;
        } finally {
            res--;
            try {
                place4 = new PlacementRC_A4("123#4567");
                res += 100;
                try {
                    place4 = new PlacementRC_A4("123#4568");
                    PlacementRC_A4 place09 = new PlacementRC_A4("123#4561");
                    res += 100;
                    place09.count = res;
                    try {
                        place4 = new PlacementRC_A4("123#4569");
                        PlacementRC_A4 place9 = new PlacementRC_A4("123#4561");
                        res += 100;
                        place9.count = res;
                        try {
                            place4 = new PlacementRC_A4("123#4560");
                            PlacementRC_A4 place18 = new PlacementRC_A4("123#4561");
                            res += 100;
                            place18.count = res;
                            place9.count = place18.count;
                            try {
                                place4 = new PlacementRC_A4("123#4561");
                                PlacementRC_A4 place19 = new PlacementRC_A4("123#4561");
                                res += 100;
                                place19.count = res;
                                place9.count = place19.count;
                                System.out.println(place4.className.substring(-5));
                            } catch (ClassCastException e) {
                                place9.className = "catch info 1";
                                res -= 10;
                            } finally {
                                res -= 100;
                            }
                        } catch (ClassCastException e1) {
                            place4.className = "catch info 2";
                            res -= 10;
                        } finally {
                            res -= 100;
                        }
                    } catch (IllegalStateException e2) {
                        place09.className = "catch info 3";
                        res -= 10;
                    } finally {
                        res -= 100;
                    }
                } catch (ClassCastException e3) {
                    place4.className = "catch info 4";
                    res -= 10;
                } finally {
                    res -= 100;
                }
            } catch (IllegalArgumentException e4) {
                place4.className = "catch info 5";
                res -= 10;
            } finally {
                res -= 100;
            }
        }
        res -= 10;
        place4.count = result1;
        return place4.count;
    }
}