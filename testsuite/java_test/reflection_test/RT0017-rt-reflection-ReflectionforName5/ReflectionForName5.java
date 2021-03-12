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


interface ForName55_c {
}
class ForName55_b implements ForName55_c {
}
class ForName55_a extends ForName55_b {
}
class ForName55 extends ForName55_a {
}
class ForName5_e {
}
class ForName5_d extends ForName5_e {
}
class ForName5_c extends ForName5_d {
}
class ForName5_b extends ForName5_c {
}
class ForName5_a extends ForName5_b {
}
class ForName5 extends ForName5_a {
}
public class ReflectionForName5 {
    public static void main(String[] args) throws ClassNotFoundException {
        Class clazz = Class.forName("ForName5");
        if (clazz.toString().equals("class ForName5")) {
            Class clazz2 = Class.forName("ForName55");
            if (clazz2.toString().equals("class ForName55")) {
                System.out.println(0);
            }
        }
    }
}