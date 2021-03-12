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


class Cast3 {
}
class Cast3_a {
}
public class ReflectionCast3 {
    public static void main(String[] args) {
        Cast3_a cast3_a = new Cast3_a();
        Cast3 cast3 = new Cast3();
        try {
            cast3_a = Cast3_a.class.cast(cast3);
        } catch (ClassCastException e) {
            System.out.println(0);
        }
    }
}