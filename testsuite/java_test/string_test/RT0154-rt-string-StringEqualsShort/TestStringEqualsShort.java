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


public class TestStringEqualsShort {
    public static void main(String args[]) throws Exception {
        StringEqualsShort test = new StringEqualsShort();
        test.resultEqualsShort();
    }
}
class StringEqualsShort {
    private final String[][] shortStrings = new String[][]{
            // Equal, constant comparison
            {"a", "a"},
            // Different constants, first character different
            {":", " :"},
            // Different constants, last character different, same length
            {"ja M", "ja N"},
            // Different constants, different lengths
            {"$$$", "$$"},
            // Force execution of code beyond reference equality check
            {"hi", new String("hi")}
    };
    public void resultEqualsShort() {
        for (int i = 0; i < shortStrings.length; i++) {
            System.out.println(shortStrings[i][0].equals(shortStrings[i][1]));
        }
    }
}
