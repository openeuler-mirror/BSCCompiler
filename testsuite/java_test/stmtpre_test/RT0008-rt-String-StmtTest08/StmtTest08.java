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


public class StmtTest08 {
    public static void main(String[] args) {
        if (test1() == false) {
            System.out.println("ExpectResult"); // 1
        } else {
            System.out.println("ExpectResult " + "ExpectResult"); // 2
        }
    }
    // 基础的测试
    private static boolean test() {
        String string = "A"; // 3
        for (int ii = 0; ii < 100; ii++) {
            string += "A";
        }
        return string.length() == 101;  //true;
    }
    // 有函数调用,确认
    private static boolean test1() {
        String str1 = "AA"; // 4
        String str2;
        if (test() == true) {
            str2 = str1;
            if (str2 == "AA") {
                str2 = str2.substring(0, 1);
            } else {
                str1 = "AAA"; // 5
            }
        } else {
            str2 = "A"; // 6
            if (str2.equals("AA")) {
                str2 = str2.substring(0, 1);
            } else {
                str1 = "AAA"; // 7
            }
        }
        return str2 == str1; //true
    }
}
