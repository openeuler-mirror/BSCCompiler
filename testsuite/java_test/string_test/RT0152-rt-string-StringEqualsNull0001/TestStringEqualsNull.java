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


public class TestStringEqualsNull {
    public static void main(String args[]) throws Exception {
        StringEqualsNull test = new StringEqualsNull();
        test.resultEqualsNull();
    }
}
class StringEqualsNull {
    private final String[] nullStrings = new String[]{
            "",
            "Hello my name is ",
            "What's your name?",
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
    private final String tmpStr1 = "012345678901234567890"
            + "0123456789012345678901234567890123456789"
            + "0123456789012345678901234567890123456789"
            + "0123456789012345678901234567890123456789"
            + "0123456789012345678901234567890123456789";
    private final String tmpStr2 = "z012345678901234567890"
            + "0123456789012345678901234567890123456789"
            + "0123456789012345678901234567890123456789"
            + "0123456789012345678901234567890123456789"
            + "012345678901234567890123456789012345678x";
    private final String[] nonalignedStrings = new String[]{
            tmpStr1.substring(1),
            tmpStr1.substring(2),
            tmpStr2.substring(4),
            tmpStr2.substring(5),
            "hello".substring(1),
    };
    public void resultEqualsNull() {
        for (int i = 0; i < nullStrings.length; i++) {
            System.out.println(nullStrings[i].equals(null));
            System.out.println(objects[i].equals(null));
            System.out.println(nonalignedStrings[i].equals(null));
        }
    }
}