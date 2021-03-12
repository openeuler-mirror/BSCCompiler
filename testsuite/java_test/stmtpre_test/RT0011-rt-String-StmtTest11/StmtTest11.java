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


public class StmtTest11 {
    public static void main(String[] args) {
        if (test()) {
            System.out.println("ExpectResult");  // 1
        } else {
            String string = "ExpectResultExpectResult"; // 2
            System.out.println(string);
        }
    }
    // 基础的测试for循环嵌套
    private static boolean test() {
        String string = "AA"; // 3
        try {
            string = "AB"; // 4
            if (string.length() == 2) {
                for (int jj = 0; jj < 10; jj++) {
                    for (int ii = 0; ii < getInt(); ii++) {
                        string += "123";  // 5,被外提
                    }
                }
            } else {
                switch (string) {
                    case "A": // 6
                        string = "123"; //被外提
                        break;
                    case "AA":
                        string = "A";
                        break;
                    default:
                        break;
                }
            }
        } catch (ArithmeticException a) {
            System.out.println(string);
        }
        return string.length() == 2;  //true;
    }
    private static int getInt() {
        return 1 / 0;
    }
}
