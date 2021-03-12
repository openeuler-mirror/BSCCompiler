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
import java.io.PrintStream;
class TemplateClassTest<T> {
    static {
        Runtime.getRuntime().gc();
    }
    public TemplateClassTest(T init) {
        field = init;
    }
    private T field;
    public void Show() {
        System.out.println(field);
    }
}
public class ClinitGC001 {
    public static void main(String[] argv) {
        ArrayList<Integer> al = new ArrayList<Integer>();
        TemplateClassTest<Integer> test = new TemplateClassTest<Integer>(0);
        try {
            test.Show();
            al.add(1);
        } catch (NullPointerException e) {
            System.out.println(2);
        }
    }
}
