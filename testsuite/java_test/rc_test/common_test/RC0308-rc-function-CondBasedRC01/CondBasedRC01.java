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


public class CondBasedRC01 {
    private String str1;
    public static String str2;
    public CondBasedRC01() {
    }
    public String testfunc(String s) {
        if (s == null) {
            s = str1;
        }
        return s;
    }
    public String strfunc() {
        return str1;
    }
    public String testfunc1() {
        String t = strfunc();
        if (t == null) {
            t = str1;
        }
        return t;
    }
    public static void main(String[] args){
        CondBasedRC01 temp1 = new CondBasedRC01();
        String s = new String("test");
        str2 = temp1.testfunc(s);
        str2 = temp1.testfunc1();
        System.out.println("ExpectResult");
    }
}
