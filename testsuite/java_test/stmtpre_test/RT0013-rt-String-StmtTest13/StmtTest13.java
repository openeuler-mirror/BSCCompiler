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


public class StmtTest13 {
    private String outOfMethod = "AB"; //3
    public static void main(String[] args) {
        String string = "AB"; // 1
        for (int ii = 0; ii < 10; ii++) {
            string += "A" + "B";
        }
        if (string.length() == 22) {
            string = "AB"; //此处在堆上分配内存，不会用第22行的伪寄存器地址
        } else {
            char[] chars = {'A', 'B'};
            string = chars.toString();
        }
        if (string.equals(new StmtTest13().outOfMethod)) {
            System.out.println("ExpectResult");  // 2
        } else {
            System.out.print("ExpectResult");  //优化外提，这边不会再取一次。
        }
    }
}
