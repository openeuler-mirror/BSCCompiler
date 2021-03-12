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


public class TestStringEqualsVeryLong {
    public static void main(String args[]) throws Exception {
        StringEqualsVeryLong test = new StringEqualsVeryLong();
        test.resultEqualsVeryLong();
    }
}
class StringEqualsVeryLong {
    private final String veryLong = "Garbage collection has two phases. The first distinguishes"
            + "live objects from garbage objects.  The second is reclaiming the rage of garbage object"
            + "In the mark-sweep algorithm used by Dalvik, the first phase is achievd by computing the"
            + "closure of all reachable objects in a process known as tracing from theoots.  After the"
            + "trace has completed, garbage objects are reclaimed.  Each of these operations can be"
            + "parallelized and can be interleaved with the operation of the applicationTraditionally,"
            + "the tracing phase dominates the time spent in garbage collection.  The greatreduction i"
            + "pause time can be achieved by interleaving as much of this phase as possible with the"
            + "application. If we simply ran the GC in a separate thread with no other changes, normal"
            + "operation of an application would confound the trace.  Abstractly, the GC walks the h o"
            + "all reachable objects.  When the application is paused, the object graph cannot change."
            + "The GC can therefore walk this structure and assume that all reachable objects live."
            + "When the application is running, this graph may be altered. New nodes may be addnd edge"
            + "may be changed.  These changes may cause live objects to be hidden and falsely recla by"
            + "the GC.  To avoid this problem a write barrier is used to intercept and record modifion"
            + "to objects in a separate structure.  After performing its walk, the GC will revisit the"
            + "updated objects and re-validate its assumptions.  Without a card table, the garbage"
            + "collector would have to visit all objects reached during the trace looking for dirtied"
            + "objects.  The cost of this operation would be proportional to the amount of live data."
            + "With a card table, the cost of this operation is proportional to the amount of updateat"
            + "The write barrier in Dalvik is a card marking write barrier.  Card marking is the proce"
            + "of noting the location of object connectivity changes on a sub-page granularity.  A car"
            + "is merely a colorful term for a contiguous extent of memory smaller than a page, common"
            + "somewhere between 128- and 512-bytes.  Card marking is implemented by instrumenting all"
            + "locations in the virtual machine which can assign a pointer to an object.  After themal"
            + "pointer assignment has occurred, a byte is written to a byte-map spanning the heap whic"
            + "corresponds to the location of the updated object.  This byte map is known as a card ta"
            + "The garbage collector visits this card table and looks for written bytes to reckon the"
            + "location of updated objects.  It then rescans all objects located on the dirty card,"
            + "correcting liveness assumptions that were invalidated by the application.  While card"
            + "marking imposes a small burden on the application outside of a garbage collection, the"
            + "overhead of maintaining the card table is paid for by the reduced time spent inside"
            + "garbage collection. With the concurrent garbage collection thread and a write barrier"
            + "supported by the interpreter, JIT, and Runtime we modify garbage collection";
    private final String[][] veryLongStrings = new String[][]{
            // Force execution of code beyond reference equality check
            {veryLong, new String(veryLong)},
            // Different constants, different lengths
            {veryLong + veryLong, veryLong + " " + veryLong},
            // Equal, constant comparison
            {veryLong + veryLong + veryLong, veryLong + veryLong + veryLong},
            // Different constants, last character different, same length
            {veryLong + "77777", veryLong + "99999"},
            // Different constants, first character different
            {"Android Runtime" + veryLong, "android Runtime" + veryLong}
    };
    // Benchmark cases with medium length (10-15 character) Strings
    public void resultEqualsVeryLong() {
        for (int i = 0; i < veryLongStrings.length; i++) {
            System.out.println(veryLongStrings[i][0].equals(veryLongStrings[i][1]));
        }
    }
}
