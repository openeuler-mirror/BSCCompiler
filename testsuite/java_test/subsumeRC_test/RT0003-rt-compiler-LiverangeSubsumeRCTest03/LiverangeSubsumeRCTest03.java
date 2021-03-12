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


public class LiverangeSubsumeRCTest03 {
    public static void main(String[] args) {
        int[] array1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        int[] array2 = array1;
        int[] array3 = array2;
        String str1 = "ExpectResult";
        String str2 = str1;
        String str3 = str2;
        boolean check = String.valueOf(array2[0]).equals(str2); // false
        System.out.print(check);
        check = String.valueOf(array3[1]) == str3;
        System.out.print(check); // false
        if (array1.length == 10) {
            System.out.println(str1);
        } else {
            System.out.print(str1); // 错误输出，无换行符
        }
    }
}
