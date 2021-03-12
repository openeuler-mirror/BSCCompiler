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
class TopLevelClass1 {
    static int field;
    static {
        if (ClinitBase001.getCount() == 3) {
            field = ClinitBase001.getCount();
        }
    }
    int getField() {
        return field;
    }
    static class StaticClass {
        static int field;
        static {
            if (ClinitBase001.getCount() == 5) {
                field = ClinitBase001.getCount();
            }
        }
        int getField() {
            return field;
        }
        static class StaticNestClass {
            static int field;
            static {
                if (ClinitBase001.getCount() == 7) {
                    field = ClinitBase001.getCount();
                }
            }
            int getField() {
                return field;
            }
            static class StaticInnerClass {
                static int field;
                static {
                    if (ClinitBase001.getCount() == 1) {
                        field = ClinitBase001.getCount();
                    }
                }
                int getField() {
                    return field;
                }
            }
        }
    }
}
public class ClinitBase001 {
    static private int count = 0;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    static int getCount() {
        count += 1;
        return count;
    }
    private static int run(String[] argv, PrintStream out) {
        int res = 0/*STATUS_PASSED*/;
        // 创建内部静态类实例，触发内部静态类初始化，不触发外部类的初始化
        TopLevelClass1.StaticClass.StaticNestClass.StaticInnerClass case4 =
                new TopLevelClass1.StaticClass.StaticNestClass.StaticInnerClass();
        // 创建外部类实例，触发外部类初始化，不触发内部类的初始化
        TopLevelClass1 case1 = new TopLevelClass1();
        // 创建内部静态类的实例，触发内部静态类初始化，不触发它内部类的初始化
        TopLevelClass1.StaticClass case2 = new TopLevelClass1.StaticClass();
        TopLevelClass1.StaticClass.StaticNestClass case3 =
                new TopLevelClass1.StaticClass.StaticNestClass();
        if (case1.getField() != 4) {
            out.println("Error: TopLevelClass1 initialized Failed");
            res = 2;
        }
        if (case2.getField() != 6) {
            out.println("Error: TopLevelClass1.StaticClass initialized Failed");
            res = 2;
        }
        if (case3.getField() != 8) {
            out.println("Error: TopLevelClass1.StaticClass.StaticNestClass initialized Failed");
            res = 2;
        }
        if (case4.getField() != 2) {
            out.println("Error: TopLevelClass1.StaticClass.StaticNestClass initialized Failed");
            res = 2;
        }
        //  new数组不触发对应类的初始化
        TopLevelClass1[] array = new TopLevelClass1[4];
        if (array.length != 4) {
            res = 2;
        }
        if (count != 8) {
            out.println("Error: class initialized should not be invoked twice");
            res = 2;
        }
        return res;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
