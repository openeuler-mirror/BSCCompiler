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


public class ThreadSetName3Test extends Thread {
    public static void main(String[] args) {
        ThreadSetName3Test testName1 = new ThreadSetName3Test();
        ThreadSetName3Test testName2 = new ThreadSetName3Test();
        testName1.start();
        testName1.setName("");
        testName2.start();
        testName2.setName("");
        if (testName1.getName().equals("")) {
            if (testName1.getName().equals("")) {
                System.out.println(0);
                return;
            }
        }
        System.out.println(2);
        return;
    }
}