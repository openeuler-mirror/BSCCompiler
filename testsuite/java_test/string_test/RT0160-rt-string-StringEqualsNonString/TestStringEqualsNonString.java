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


public class TestStringEqualsNonString {
    public static void main(String args[]) throws Exception {
        StringEqualsNonString test = new StringEqualsNonString();
        test.resultEqualsNonString();
    }
}
class StringEqualsNonString {
    private final String[] Strings = new String[]{
            "Hello my name is ",
            "What's your name?",
            "Android Runtime",
            "v3ry Cre@tiVe?****",
            "!@#$%^&*()_++*^$#@",
    };
    private final Object[] objects = new Object[]{
            // Compare to Double object
            new Double(1.5),
            // Compare to Integer object
            new Integer(9999999),
            // Compare to String array
            new String[]{"h", "i"},
            // Compare to int array
            new int[]{1, 2, 3},
            // Compare to Character object
            new Character('a')
    };
    public void resultEqualsNonString() {
        for (int i = 0; i < Strings.length; i++) {
            System.out.println(Strings[i].equals(objects[i]));
        }
    }
}
