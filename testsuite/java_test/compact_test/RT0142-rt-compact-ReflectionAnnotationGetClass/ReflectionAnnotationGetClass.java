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
import java.util.Arrays;
public class ReflectionAnnotationGetClass {
    public static void main(String[] args) {
        int result = 2;
        try{
            // Check point 1 : only 1 annotation in user-defined Class Two.
            Annotation[] annotations1 = Class.forName("Two").getAnnotations();
            if (annotations1.length == 1){
                if (annotations1[0].getClass().toString().startsWith("class $Proxy")) {
                    result --;
                }
            }
            // Check point 2 : 3 annotations in Class java.lang.annotation.Documented.
            Annotation[] annotations2 = Class.forName("java.lang.annotation.Documented").getAnnotations();
            String [] annoClazz = new String[annotations2.length];
            for (int i = 0; i < annoClazz.length; i++) {
                annoClazz[i] = annotations2[i].getClass().toString();
            }
            Arrays.sort(annoClazz);
            if (annotations2.length == 3) {
                if (annoClazz[0].startsWith("class $Proxy") && annoClazz[1]
                        .startsWith("class $Proxy")
                        && annoClazz[2].startsWith("class $Proxy")) {
                    result --;
                }
            }
        }catch (Exception e) {
            result = 3;
        }
        System.out.println(result);
    }
}
@A0
class Two{}
@Retention(RetentionPolicy.RUNTIME) @interface A0 {}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
