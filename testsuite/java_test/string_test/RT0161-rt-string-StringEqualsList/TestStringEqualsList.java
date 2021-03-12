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


import java.util.ArrayList;
public class TestStringEqualsList {
    public static void main(String[] args) {
        String str1 = "Amy";
        ArrayList<String> list = new ArrayList<>();
        list.add("Amy");
        list.add("Jack");
        list.add("Pearl");
        list.add("Rose");
        for (String str : list) {
            if (str.equals(str1)) {
                System.out.println("Amy is present");
            }
        }
    }
}