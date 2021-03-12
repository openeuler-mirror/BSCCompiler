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


import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
public class SoftRefTest03 {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static int a = 100;
    static void setSoftReference() {
        rp = new SoftReference<Object>(new Object(), rq);
        if (rp.get() == null) {
            System.out.println("Error Result when first check ");
            a++;
        }
    }
    public static void main(String[] args) throws Exception {
        setSoftReference();
        for (int i = 0; i < 3; i++) {
            Runtime.getRuntime().gc();
            Runtime.getRuntime().runFinalization();
            if (rp.get() == null) {
                System.out.println("Error Result when second check ");
                a++;
                break;    //rp指向的对象不应该被释放，如果出现释放，则打印、a++;然后break跳出循环
            }
        }
        Reference r = rq.poll();    //ReferenceQueue里不应该有对象，poll返回值应该是空指针
        if (r != null) {
            System.out.println("Error Result when checking ReferenceQueue");
            a++;
        }
        if (a == 100) {
            System.out.println("ExpectResult");
        } else {
            System.out.println("ErrorResult finally");
        }
    }
}
