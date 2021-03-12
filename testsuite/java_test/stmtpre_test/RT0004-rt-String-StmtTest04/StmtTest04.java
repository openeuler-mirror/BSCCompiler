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


class Father {
    public String name;
    public boolean test1() {
        String str1 = "A" + "A"; // 1
        String str2 = "A" + "A";
        return str1 == str2; // false;
    }
}
public class StmtTest04 extends Father {
    public String name;
    public static void main(String[] args) {
        Father father = new StmtTest04();
        father.name = "ExpectResult"; // 2
        if (father.test1()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ExpectResult" + " " + "ExpectResult"); // 4
        }
    }
    // 集成父类
    @Override
    public boolean test1() {
        String str1 = "A" + "A"; // 3
        String str2 = "A" + "A";
        return str1.equals(str2); // true;
    }
}
