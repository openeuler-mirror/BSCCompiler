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
import java.io.FileInputStream;
import java.lang.ref.WeakReference;
public class TriggerGCTest11 {
    static WeakReference observer;
    static int check = 2;
    public static String str = new String("Native memory test");
    public static int[] arr = new int[10 * 1024 * 1024];
    public static void main(String[] args) throws Exception {
        observer = new WeakReference<Object>(new Object());
        if (observer.get() != null) {
            check--;
        }
        rc_testcase_main_wrapper();
        Thread.sleep(3000);
        if (observer.get() == null) {
            check--;
        }
        if (check == 0) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult, check should be 0, but now check = " + check);
        }
    }
    private static void rc_testcase_main_wrapper() {
        try {
            String filePath = "/system/lib64/libmaplecore-all.so";
            FileInputStream fis = new FileInputStream(filePath); // Any file with size >= 501*501*501*2
            int fileSize = fis.available();
            byte buf[] = new byte[fileSize];
            fis.read(buf);
            System.out.println("buffRead ok");
        } catch (Exception e) {
            // do nothing
        }
    }
}
