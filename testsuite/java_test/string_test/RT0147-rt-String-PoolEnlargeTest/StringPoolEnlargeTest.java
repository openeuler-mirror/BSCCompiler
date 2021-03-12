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


import java.io.PrintStream;
public class StringPoolEnlargeTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String[] argv,PrintStream out){
        int result = 2;  /* STATUS_Success*/

        try {
            result = result - StringPoolEnlargeTest_1(47000);
            result = result - StringPoolEnlargeTest_1(470000);
        } catch(Exception e){
            System.out.println(e);
            StringPoolEnlargeTest.processResult = StringPoolEnlargeTest.processResult - 10;
        }
        if (result == 0 && StringPoolEnlargeTest.processResult == 95){
            result = 0;
        }
        return result;
    }
    public static int StringPoolEnlargeTest_1(int len) {
        int length = len; // 47 mapleStringPool/process, 1000 key-value/mapleStringPool
        try {
            String[] s;
            s = new String[length];
            for (int i = 0; i < length; i++){
                s[i] = Integer.toHexString(i);
                if (s[i].intern() == Integer.toHexString(func1(len-1)).intern()) {
                    StringPoolEnlargeTest.processResult = StringPoolEnlargeTest.processResult - 2;
                    return 1;
                }
            }
        } catch (OutOfMemoryError e) {
            return 114;
        }
        StringPoolEnlargeTest.processResult = StringPoolEnlargeTest.processResult - 10;
        return 102;
    }
    public static int func1(int i){
        return i;
    }
}
