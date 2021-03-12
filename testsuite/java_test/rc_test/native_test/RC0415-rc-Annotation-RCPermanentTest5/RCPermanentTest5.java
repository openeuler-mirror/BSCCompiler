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
import sun.misc.Cleaner;
import java.lang.ref.*;
import static java.lang.Thread.sleep;
class FinalizableObject {
    // static int value;
    public void finalize() {
        System.out.println("ExpectResult");
    }
}
public class RCPermanentTest5 {
    static {
        System.loadLibrary("jniTestHelper");
    }
    public static native boolean isHeapObject(Object obj);
    public static native int refCount(Object obj);
    static Object owner;
    static int checkSum;
    static Object referent = new Object();
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
    }
    static void methodRef1(Object obj) {
        obj = new @Permanent WeakReference(referent);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }
    static void methodRef2(Object obj) {
        obj = new @Permanent SoftReference(referent);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }
    static void methodRef3(Object obj) {
        ReferenceQueue rq = new ReferenceQueue();
        obj = new @Permanent PhantomReference(referent, rq);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }
    static void methodFinal(Object obj) throws InterruptedException {
        obj = new @Permanent FinalizableObject();
        sleep(3000);
        boolean result1 = checkRC(obj);
        boolean result2 = isHeapObject(obj);
        if (String.valueOf(result1).equals("true") && String.valueOf(result2).equals("true")) {
            checkSum++;
        } else {
            System.out.println("resultRC:" + result1);
            System.out.println("result_isHeap:" + result2);
        }
    }
    public static void main(String[] args) throws InterruptedException {
        Object obj = null;
        methodRef1(obj);
        methodRef2(obj);
        methodRef3(obj);
        methodFinal(obj);
        if (checkSum == 4) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("error");
            System.out.println("checkNum:" + checkSum);
        }
    }
}
