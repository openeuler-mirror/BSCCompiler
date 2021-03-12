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


public class StmtTest09 {
    public static void main(String[] args) {
        String test = "TestStringForStmt"; // 1
        switch (test) {
            case "TestStringForStmt33": // 2
                test = "TestStringForStmt01"; // 3
                break;
            case "TestStringForStmt55":  // 4
                test += "TestStringForStmt01"; // 其他优化导致"TestStringForStmt01"的地址调用外提到22行，所以与33行合用。
                break;
            default:
                switch (test) {
                    case "TestStringForStmt34": // 5
                        test = "TestStringForStmt01"; // 其他优化导致"TestStringForStmt01"的地址调用外提到22行，所以与33行合用
                        break;
                    case "TestStringForStmt35":  // 6
                        test += "TestStringForStmt01"; // 7
                        break;
                    default:
                        test = "TestStringForStmt";
                        break;
                }
        }
        String output = "ExpectResult"; // 8、
        if (test == "TestStringForStmt") {
            System.out.println(output);
        } else {
            System.out.print("ExpectResult");
        }
    }
}
