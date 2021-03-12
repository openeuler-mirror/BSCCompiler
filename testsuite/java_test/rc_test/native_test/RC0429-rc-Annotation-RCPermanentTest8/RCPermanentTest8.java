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


import java.util.ArrayList;
import com.huawei.ark.annotation.Permanent;
import java.lang.ref.WeakReference;
public class RCPermanentTest8 {
    static {
        System.loadLibrary("jniTestHelper");
    }
    public static native boolean isHeapObject(Object obj);
    public static native int refCount(Object obj);
    public static int checkNum = 0;
    static Object owner;
    static boolean checkRC(Object obj) {
        int rc1 = refCount(obj);
        owner = obj;
        int rc2 = refCount(obj);
        owner = null;
        int rc3 = refCount(obj);
        if (rc1 != rc3) {
            throw new RuntimeException("rc incorrect!");
        }
        if (rc1 == rc2 && rc2 == rc3) {
            //如果相等，说明annotation生效，没有经过RC处理
            return false;
        } else {
            return true;
        }
        //System.out.printf("rc:%-5s heap:%-5s %s%n", !skipRC, isHeapObject(obj), title);
    }
    /*
    验证new int @Permanent [8]
   */


    static void method1(Object obj) {
        try {
            obj = new int@Permanent[8];
            boolean result1 = checkRC(obj);
            if (result1 == false && isHeapObject(obj) == false) {
                checkNum++;
            } else {
                System.out.println("error in new int @Permanent [8];in method1");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    public static void main(String[] args) {
        Object obj = null;
        method1(obj);
        if (checkNum == 1) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("checkNum:" + checkNum);
        }
    }
}
