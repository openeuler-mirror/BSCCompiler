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


public class T7040104 {
    int npeCount = 0;
    public static void main(String[] args) {
        T7040104 t = new T7040104();
        t.test1();
        t.test2();
        t.test3();
        if (t.npeCount != 3) {
            System.out.println("error");
        } else {
            System.out.println("ExpectResult");
        }
    }
    void test1() {
        Object[] a;
        try {
            Object o = (a = null)[0];
        } catch (NullPointerException npe) {
            npeCount++;
        }
    }
    void test2() {
        Object[][] a;
        try {
            Object o = (a = null)[0][0];
        } catch (NullPointerException npe) {
            npeCount++;
        }
    }
    void test3() {
        Object[][][] a;
        try {
            Object o = (a = null)[0][0][0];
        } catch (NullPointerException npe) {
            npeCount++;
        }
    }
}
