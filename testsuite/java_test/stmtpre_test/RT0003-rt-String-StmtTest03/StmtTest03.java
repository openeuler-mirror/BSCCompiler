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


interface Interface {
    boolean test();
}
public class StmtTest03 implements Interface {
    public static void main(String[] args) {
        if (new StmtTest03().test()) {
            System.out.println("ExpectResult");  // 1
        } else {
            System.out.println("ExpectResult " + "ExpectResult"); // 2
        }
    }
    // 实现接口
    public boolean test() {
        String str1 = "A" + "A"; // 3
        String str2 = "A" + "A";
        return str1 == str2;
    }
}
