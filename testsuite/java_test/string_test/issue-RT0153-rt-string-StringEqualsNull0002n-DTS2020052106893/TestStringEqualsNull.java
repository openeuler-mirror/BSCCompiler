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


import java.io.PrintWriter;
import java.io.StringWriter;
public class TestStringEqualsNull {
    public static void main(String args[]) throws Exception {
        StringEqualsNull test = new StringEqualsNull();
        test.resultEqualsNull();
    }
}
class StringEqualsNull {
    private final String[] nullStrings = new String[]{null};
    public void resultEqualsNull() {
        int result = 1; /* STATUS_FAILED*/

        try {
            System.out.println(nullStrings[0].equals(null));
        } catch (Exception e) {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            e.printStackTrace(pw);
            String msg = sw.toString();
            if (msg.contains("java.lang.NullPointerException")) {
                result--;
            }
            System.out.println(result);
        }
    }
}
