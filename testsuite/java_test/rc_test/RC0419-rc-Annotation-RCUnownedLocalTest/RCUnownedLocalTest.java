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


import com.huawei.ark.annotation.UnownedLocal;
import java.util.LinkedList;
import java.util.List;
public class RCUnownedLocalTest {
    static Integer a = new Integer(1);
    static Object[] arr = new Object[]{1, 2, 3};
    @UnownedLocal
    static int method(Integer a, Object[] arr) {
        int check = 0;
        Integer c = a + a;
        if (c == 2) {
            check++;
        } else {
            check--;
        }
        for (Object array : arr) {
            //System.out.println(array);
            check++;
        }
        return check;
    }
    public static void main(String[] args) {
        int result = method(a, arr);
        if (result == 4) {
            System.out.println("ExpectResult");
        }
    }
}
