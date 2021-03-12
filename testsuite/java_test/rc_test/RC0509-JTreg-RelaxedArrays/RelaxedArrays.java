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
import java.util.List;
public class RelaxedArrays {
    static <T> T select(T... tl) {
        return tl.length == 0 ? null : tl[tl.length - 1];
    }
    public static void main(String[] args) {
        List<String>[] a = new StringList[20];
        if (select("A", "B", "C") != "C") {
            System.out.println("error");
        } else {
            System.out.println("ExpectResult");
        }
    }
    static class StringList extends ArrayList<String> {
    }
}