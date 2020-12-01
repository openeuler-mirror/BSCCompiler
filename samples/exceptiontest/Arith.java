/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
public class Arith {
    private static native void raise_sigfpe();

    public static int TestMain( int c ) {
        int r = 0;
        try {
            r += 1;
            if( c > 0 )
                raise_sigfpe();
            else if ( c == 0 ) {
                r += 103;
                throw new ArithmeticException();
            }
            r += 3;
        } catch( ArithmeticException e ) {
            r += 100;
        }
        return r;
    }

    public static void main(String[] args) {
        System.out.println(TestMain(-5));
        System.out.println(TestMain(0));
    }
}
