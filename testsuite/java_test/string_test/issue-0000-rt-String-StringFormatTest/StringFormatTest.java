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
import java.util.Date;
import java.util.Locale;
public class StringFormatTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }
    public static int run(String argv[],PrintStream out){
        int result = 3/*STATUS_FAILED*/;
        try {
            result = StringFormatTest_1(new Date());
//            System.out.println(String.format(Locale.ENGLISH, "%tA", new Date()));
//            System.out.println(String.format(Locale.CHINESE, "%tA", new Date()));
//            System.out.println(String.format(Locale.FRENCH, "%tA", new Date()));
        } catch(Exception e){
            System.out.println(e);
            processResult = processResult - 10;
        }
//        System.out.println("result: " + result);
//        System.out.println("processResult:" + processResult);
        if (result == 2 && processResult == 99){
            result =0;
        }
        return result;
    }
    private static int StringFormatTest_1(Object obj) {
        int result1 = 3/*STATUS_FAILED*/;
        if (String.format(Locale.ENGLISH, "%tA", obj).equals("Monday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期一") && String.format(Locale.FRENCH, "%tA", obj).equals("lundi")) {
                return 2;
            }
        } else if (String.format(Locale.ENGLISH, "%tA", obj).equals("Tuesday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期二") && String.format(Locale.FRENCH, "%tA", obj).equals("mardi")) {
                return 2;
            }
        } else if (String.format(Locale.ENGLISH, "%tA", obj).equals("Wednesday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期三") && String.format(Locale.FRENCH, "%tA", obj).equals("mercredi")) {
                return 2;
            }
        } else if (String.format(Locale.ENGLISH, "%tA", obj).equals("Thursday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期四") && String.format(Locale.FRENCH, "%tA", obj).equals("jeudi")) {
                return 2;
            }
        } else if (String.format(Locale.ENGLISH, "%tA", obj).equals("Friday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期五") && String.format(Locale.FRENCH, "%tA", obj).equals("vendredi")) {
                return 2;
            }
        } else if (String.format(Locale.ENGLISH, "%tA", obj).equals("Saturday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期六") && String.format(Locale.FRENCH, "%tA", obj).equals("samedi")) {
                return 2;
            }
        } else if (String.format(Locale.ENGLISH, "%tA", obj).equals("Sunday")) {
            if (String.format(Locale.CHINESE, "%tA", obj).equals("星期日") && String.format(Locale.FRENCH, "%tA", obj).equals("dimanche")) {
                return 2;
            }
        } else {
            return result1;
        }
        return result1;
    }
}
