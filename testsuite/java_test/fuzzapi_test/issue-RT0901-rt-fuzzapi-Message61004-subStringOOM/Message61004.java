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


public class Message61004 {
    private static int RES = 99;
    private static Class<?> clazz;
    public static void main(String[] args) {
//message61104();
        int result = 2;
        try {
            result = message61004();
            if(result == 4 && Message61004.RES == 89){
                result = 0;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println(result);
    }
    /*    测试实例：测试入参：1389308892测试入参：-1727641912测试api信息：java.lang.String测试api信息：substring测试api信息：int测试api信息：int
    */

    private static int message61004() {
        String str = null;
        try {
            clazz = Class.forName("java.lang.String");
//            Method[] met = clazz.getMethods();
//            Method method = clazz.getMethod("substring", int.class, int.class);
            int parameters1 = 1389308892;
            int parameters2 = -1727641912;
            String instance = new String();
            instance.substring(parameters1, parameters2);
//            Object result1 = method.invoke(instance, parameters1, parameters2);
        } catch (Error e) {
            str = e.getClass().toString();
            //System.out.println(str);
            //e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if(str != null && str.equals("class java.lang.OutOfMemoryError")){
            Message61004.RES -= 10;
        }
        return 4;
    }
}
