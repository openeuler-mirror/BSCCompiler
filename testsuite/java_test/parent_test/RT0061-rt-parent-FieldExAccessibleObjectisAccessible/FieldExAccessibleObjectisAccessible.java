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


import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectisAccessible {
    static int res = 99;
    public static void main(String[] argv) {
        System.out.println(new FieldExAccessibleObjectisAccessible().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectisAccessible1();
        } catch (Exception e) {
            FieldExAccessibleObjectisAccessible.res = FieldExAccessibleObjectisAccessible.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectisAccessible.res == 89) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectisAccessible1() throws NoSuchFieldException {
        //  boolean isAccessible()
        int result1 = 4;
        Field f1 = SampleClassFieldH2.class.getDeclaredField("id");
        try {
            if(!f1.isAccessible()){
                FieldExAccessibleObjectisAccessible.res = FieldExAccessibleObjectisAccessible.res - 10;
            }else{
                FieldExAccessibleObjectisAccessible.res = FieldExAccessibleObjectisAccessible.res - 15;
            }
        } catch (Exception e) {
            FieldExAccessibleObjectisAccessible.res = FieldExAccessibleObjectisAccessible.res - 15;
        }
        return result1;
    }
}
class SampleClassFieldH2 {
    @CustomAnnotationsH2(name = "id")
    String id;
}
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@interface CustomAnnotationsH2 {
    String name();
}
