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

import java.lang.annotation.*;
import java.lang.reflect.*;

@Retention(RetentionPolicy.RUNTIME)
@interface HarmonyStaticStringVis {

}

@HarmonyStaticStringVis
class HelloWorldAnnotation {
    @HarmonyStaticStringVis
    private int numVis;

    @HarmonyStaticStringVis
    public void funcVis(@HarmonyStaticStringVis String str, int num, @HarmonyStaticStringVis String str2) {
        for (int i = 0; i < num; ++i) {
            System.out.println(str);
            System.out.println(str2);
        }
    }
}

public class HelloWorld {
    public static void main(String[] args) {
        funcVisTest();
    }

    public static void funcVisTest() {
        try {
            Class klass = Class.forName("HelloWorldAnnotation");
            Method[] methods = klass.getMethods();
            Method funcVis = null;
            for (int i = 0; i < methods.length; i++) {
                if (methods[i].getName().equals("funcVis")) {
                    funcVis = methods[i];
                    break;
                }
            }
            Parameter[] parameters = funcVis.getParameters();
            Parameter p0 = parameters[0];
            Parameter p1 = parameters[1];
            Parameter p2 = parameters[2];
            if ((p0.getAnnotation(HarmonyStaticStringVis.class) != null) &&
                (p1.getAnnotation(HarmonyStaticStringVis.class) == null) &&
                (p2.getAnnotation(HarmonyStaticStringVis.class) != null)) {
              System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
