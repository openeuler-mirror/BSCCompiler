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


public class DoubleSynchronized {
    private byte[] test = new byte[0];
    private int[] test2 = new int[0];
    private String string = null;
    public void test () {
        try {
            synchronized(test) {
                System.out.println(string.length());
            }
            synchronized(test2) {
                System.out.println(string.length());
            }
        } catch (NullPointerException e) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
    }
    public static void main(String [] args) {
        new DoubleSynchronized().test();
    }
}
