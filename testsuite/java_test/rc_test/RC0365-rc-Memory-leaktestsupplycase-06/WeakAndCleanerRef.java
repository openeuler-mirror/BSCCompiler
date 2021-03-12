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


import sun.misc.Cleaner;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
public class WeakAndCleanerRef {
    static Reference rp;
    static ReferenceQueue rq = new ReferenceQueue();
    static StringBuffer stringBuffer = new StringBuffer("test");
    static void addWeakAndCleanerRef() {
        rp = new WeakReference(stringBuffer, rq);
    }
    public static void main(String[] args) throws InterruptedException {
        ThreadCleaner threadCleaner = new ThreadCleaner();
        Cleaner cleaner = Cleaner.create(stringBuffer, threadCleaner);
        addWeakAndCleanerRef();
        stringBuffer = null;
        cleaner.clean();
        Thread.sleep(2000);
        addWeakAndCleanerRef();
        if (rp.get() == null) {
            System.out.println("ExpectResult");
        }
    }
}
class ThreadCleaner implements Runnable {
    @Override
    public void run() {
    }
}