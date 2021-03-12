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


public class LiverangeSubsumeRCTest14 {
    public static void main(String[] args) {
        String str1 = "ExpectResult";
        String str2 = str1;
        try {
            String temp = str2.substring(0, 3);
            temp = str1.toLowerCase();
            str1 = str2;
            temp = temp.equals("ExpectResult") ? temp : str1;
            temp.notifyAll();
            str1.notifyAll();
        } catch (Exception e) {
            System.out.print(str1);
        } finally {
            System.out.println(str1);
        }
    }
}
