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


import java.io.File;
import java.io.IOException;
class ForName3 {
    static {
        File cls2 = new File("/data/local/tmp/ReflectionForName3.txt");
        try {
            cls2.createNewFile();
        } catch (IOException e) {
            System.err.println(e);
        }
    }
}
public class ReflectionForName3 {
    public static void main(String[] args) throws ClassNotFoundException {
        File cls1 = new File("/data/local/tmp/ReflectionForName3.txt");
        if (cls1.exists()) {
            cls1.delete();
        }
        Class cls = Class.forName("ForName3", false, ForName3.class.getClassLoader());
        if (!cls1.exists()) {
            System.out.println(0);
            return;
        }
        System.out.println(2);
        return;
    }
}
