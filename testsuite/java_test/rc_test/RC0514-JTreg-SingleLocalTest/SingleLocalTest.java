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


public class SingleLocalTest {
    static F f;
    public static void main(String[] args) {
        StringBuffer sb = new StringBuffer();
        class Local1 {
            public Local1() {
                f = () -> new Local1();
                sb.append("1");
            }
        }
        new Local1();
        f.f();
        String s = sb.toString();
        if (!s.equals("11")) {
            System.out.println("Expected '11' got '" + s + "'");
        } else {
            System.out.println("ExpectResult");
        }
    }
    interface F {
        void f();
    }
}
