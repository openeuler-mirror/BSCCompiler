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


import java.nio.charset.StandardCharsets;
public class StmtTest14 {
    public static void main(String[] args) {
        String string = "AB"; // 1
        for (int ii = 0; ii < 10; ii++) {
            string += "A" + "B"; // +号拼接场景
        }
        if (string.length() == 22) {
            string = "AB"; // 作为入参
        } else {
            char[] chars = "AB".toCharArray(); // 作为函数调用主体
            string = chars.toString();
        }
        byte[] bs = string.getBytes();
        string = new String(bs, StandardCharsets.US_ASCII); 
        System.out.println(string);
    }
}
