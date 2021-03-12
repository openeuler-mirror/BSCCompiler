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


import java.nio.charset.IllegalCharsetNameException;
public class RC_eh_01 {
    static int res = 0;
    public static void main(String argv[]) {
        run1();
		System.out.println("ExpectResult");
    }
    public static void run1() {
        try {
            new RC_eh_01().run2();
        } catch (IllegalArgumentException e) {
            RC_eh_01.res = 1;
        }
    }
    public  void run2() {
        Foo f1 = new Foo(1);
        Foo f2 = new Foo(2);
        run3();
        int result=f1.i+f2.i;
    }
    public static void run3() {
        try {
            Integer.parseInt("123#456");
        } catch (IllegalCharsetNameException e) {
            RC_eh_01.res = 3;
        }
    }
    class Foo{
        public int i;
        public Foo(int i){
            this.i=i;
        }
    }
}
