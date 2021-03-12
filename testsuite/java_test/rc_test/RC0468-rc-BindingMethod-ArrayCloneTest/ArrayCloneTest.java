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


public class ArrayCloneTest {
    public static void main(String[] args) {
        if (functionTest01()) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult");
        }
    }
    private static boolean functionTest01() {
        String[] test1 = {"test1","test2","test3","test4","test5","test6","test7","test8","test9","test10"};
        String[] test2 = test1.clone();
        if (test2 != test1  && test2.length == 10 && test2[0].equals("test1") && test2[9].equals("test10")) {
            return true;
        }
        return false;
    }
}
