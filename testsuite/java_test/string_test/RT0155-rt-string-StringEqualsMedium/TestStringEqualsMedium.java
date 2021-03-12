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


public class TestStringEqualsMedium {
    public static void main(String args[]) throws Exception {
        StringEqualsMedium test = new StringEqualsMedium();
        test.resultEqualsMedium();
    }
}
class StringEqualsMedium {
    private final String[][] mediumStrings = new String[][]{
            // Equal, constant comparison
            {"Hello my name is ", "Hello my name is "},
            // Different constants, different lengths
            {"What's your name?", "Whats your name?"},
            // Force execution of code beyond reference equality check
            {"Android Runtime", new String("Android Runtime")},
            // Different constants, last character different, same length
            {"v3ry Cre@tiVe?****", "v3ry Cre@tiVe?***."},
            // Different constants, first character different, same length
            {"!@#$%^&*()_++*^$#@", "0@#$%^&*()_++*^$#@"}
    };
    public void resultEqualsMedium() {
        for (int i = 0; i < mediumStrings.length; i++) {
            System.out.println(mediumStrings[i][0].equals(mediumStrings[i][1]));
        }
    }
}
