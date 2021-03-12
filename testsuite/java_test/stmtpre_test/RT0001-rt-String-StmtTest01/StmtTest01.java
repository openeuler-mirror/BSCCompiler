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


public class StmtTest01 {
    public static void main(String[] args) {
        if (test()) {
            System.out.println("ExpectResult");  // 1
        } else {
            String string = "ExpectResult" + "ExpectResult"; // 2
            System.out.println(string);
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
}
