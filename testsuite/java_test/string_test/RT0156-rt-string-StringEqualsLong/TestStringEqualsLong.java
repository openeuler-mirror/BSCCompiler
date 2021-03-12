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


public class TestStringEqualsLong {
    public static void main(String args[]) throws Exception {
        StringEqualsLong test = new StringEqualsLong();
        test.resultEqualsLong();
    }
}
class StringEqualsLong {
    private final String long1 = "Ahead-of-time compilation is possible as the compiler may just"
            + "convert an instruction thus: dex code: add-int v1000, v2000, v3000 C code: setIntRegter"
            + "(1000, call_dex_add_int(getIntRegister(2000), getIntRegister(3000)) This means even lid"
            + "instructions may have code generated, however, it is not expected that code generate in"
            + "this way will perform well. The job of AOT verification is to tell the compiler that"
            + "instructions are sound and provide tests to detect unsound sequences so slow path code"
            + "may be generated. Other than for totally invalid code, the verification may fail at AOr"
            + "run-time. At AOT time it can be because of incomplete information, at run-time it can e"
            + "that code in a different apk that the application depends upon has changed. The Dalvik"
            + "verifier would return a bool to state whether a Class were good or bad. In ART the fail"
            + "case becomes either a soft or hard failure. Classes have new states to represent that a"
            + "soft failure occurred at compile time and should be re-verified at run-time.";
    private final String[][] longStrings = new String[][]{
            // Force execution of code beyond reference equality check
            {long1, new String(long1)},
            // Different constants, last character different, same length
            {long1 + "fun!", long1 + "----"},
            // Equal, constant comparison
            {long1 + long1, long1 + long1},
            // Different constants, different lengths
            {long1 + "123456789", long1 + "12345678"},
            // Different constants, first character different, same length
            {"Android Runtime" + long1, "android Runtime" + long1}
    };
    public void resultEqualsLong() {
        for (int i = 0; i < longStrings.length; i++) {
            System.out.println(longStrings[i][0].equals(longStrings[i][1]));
        }
    }
}
