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
public class OOMtest {
    private static ArrayList<byte[]> store;
    private static int alloc_test() {
        int sum = 0;
        store = new ArrayList<byte[]>();
        byte[] temp;
        for (int i = 1024 * 1024 * 10; i <= 1024 * 1024 * 10; ) {
            temp = new byte[i];
            store.add(temp);
            sum += store.size();
        }
        return sum;
    }
    public static void main(String[] args) {
        try {
            int result = alloc_test();
        } catch (OutOfMemoryError o) {
            System.out.println("ExpectResult");
        }
        if (store == null) {
            System.out.println("Error");
        } else {
			System.out.println("ExpectResult");
		}
    }
}
