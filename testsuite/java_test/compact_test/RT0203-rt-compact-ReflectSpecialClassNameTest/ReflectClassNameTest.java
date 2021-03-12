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


import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;
public class ReflectClassNameTest {
    static int[] passCnt = new int[6];
    public static void main(String[] args) throws ClassNotFoundException, NoSuchMethodException, NoSuchFieldException {
        int i = 0;
        Class clazz = Class.forName("一");
        passCnt[i++] = clazz.getName().compareTo("一") == 0 ? 1 : 0;
        Method method = clazz.getDeclaredMethod("дракон");
        passCnt[i++] = method.getName().compareTo("дракон") == 0 ? 1 : 0;
        Field field = clazz.getDeclaredField("Rồng");
        passCnt[i++] = field.getName().compareTo("Rồng") == 0 ? 1 : 0;
        Class clazz2 = Class.forName("二");
        passCnt[i++] = clazz2.getName().compareTo("二") == 0 ? 1 : 0;
        class 三{
        }
        Class clazz3 = Class.forName("ReflectClassNameTest$1三");
        passCnt[i++] = clazz3.getName().compareTo("ReflectClassNameTest$1三") == 0 ? 1 : 0;
        Class clazz4 = Class.forName("ReflectClassNameTest$二");
        passCnt[i++] = clazz4.getName().compareTo("ReflectClassNameTest$二") == 0 ? 1 : 0;
        System.out.println(Arrays.toString(passCnt).compareTo("[1, 1, 1, 1, 1, 1]"));
    }
    class 二{
    }
}
class 一{
    int Rồng;
    public void дракон(){
    }
}
interface 二{
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
