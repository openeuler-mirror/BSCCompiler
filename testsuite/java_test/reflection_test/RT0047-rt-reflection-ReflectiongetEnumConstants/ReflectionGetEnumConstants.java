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


enum Weekday {
    MONDAY {
        Object test1() {
            class classA {
            }
            return new classA();
        }
    },
    TUESDAY {
        Object test2() {
            class classB {
            }
            return new classB();
        }
    },
    WEDNESDAY {
        Object test3() {
            class classC {
            }
            return new classC();
        }
    },
    THURSDAY {
        Object test4() {
            class classD {
            }
            return new classD();
        }
    },
    FRIDAY {
        Object test5() {
            class classE {
            }
            return new classE();
        }
    },
    SATURDAY {
        Object test6() {
            class classF {
            }
            return new classF();
        }
    },
    SUNDAY {
        Object test7() {
            class classG {
            }
            return new classG();
        }
    }
}
class GetEnumConstants {
}
public class ReflectionGetEnumConstants {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("Weekday");
            Class clazz2 = Class.forName("GetEnumConstants");
            Object[] objects1 = clazz1.getEnumConstants();
            Object[] objects2 = clazz2.getEnumConstants();
            if (objects1.length == 7) {
                if (objects2 == null) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}