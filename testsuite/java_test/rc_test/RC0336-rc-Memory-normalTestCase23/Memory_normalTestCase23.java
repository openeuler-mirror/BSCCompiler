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
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.Set;
public class Memory_normalTestCase23 {
    static final int PAGE_SIZE = 4016;
    static final int PAGE_HEADSIZE = 80;
    static final int OBJ_MAX_SIZE = 1008;
    static final int OBJ_HEADSIZE = 16;
    private static final int DEFAULT_THREAD_NUM = 80;
    static final int MAX_SLOT_NUM = 62;
    private static boolean mRest = false;
    private static int mRestTime = 5000;
    static boolean mRunning = true;
    static final int LOCAL_MAX_IDX = 15;
    public static final int LOCAL_MIN_IDX = 0;
    public static final int GLOBAL_MAX_IDX = 62;
    static final int GLOBAL_MIN_IDX = 16;
    static final int LARGE_PAGE_SIZE = 4096;
    static final int LARGEOBJ_CHOSEN_NUM = 2;
    static final int LARGEOBJ_ALLOC_SIZE = 128 * LARGE_PAGE_SIZE;
    private static final int DEFAULT_REPEAT_TIMES = 1;
    private static final int ALLOC_16K = 16 * 1024;
    private static final int ALLOC_12K = 12 * 1024;
    private static final int ALLOC_8K = 8 * 1024;
    private static final int ALLOC_4K = 4 * 1024;
    private static final int ALLOC_2K = 2 * 1024;
    private static final int DEFAULT_THREAD_NUM_HALF = DEFAULT_THREAD_NUM / 2;
    public static final int MAX_REALIVE_OBJ_NUM = 1000;
    static List<List<AllocUnit>> mAllThreadReference = new ArrayList<List<AllocUnit>>();
    static AllocUnit[] mLocks = new AllocUnit[DEFAULT_THREAD_NUM];
    private static int getSlotSize(int slot) {
        return MemAlloc.idx_to_size(slot);
    }
    private static void resetTestEnvirment(List<List<AllocUnit>> referenceList) {
        int size = 0;
        synchronized (referenceList) {
            size = referenceList.size();
        }
        for (; size < 2 * DEFAULT_THREAD_NUM; size++) {
            synchronized (referenceList) {
                referenceList.add(null);
            }
        }
        int i = 0;
        synchronized (referenceList) {
            for (List<AllocUnit> all : referenceList) {
                if (all != null) {
                    all.clear();
                }
                referenceList.set(i++, null);
            }
        }
    }
    private static Set<Integer> getRandomIndex(List<AllocUnit> arrayToFree, int freeNum) {
        HashSet<Integer> result = new HashSet<Integer>();
        if (arrayToFree == null) {
            return result;
        }
        randomSet(0, arrayToFree.size(), freeNum >= arrayToFree.size() ? arrayToFree.size() : freeNum, result);
        return result;
    }
    private static void randomSet(int min, int max, int n, HashSet<Integer> set) {
        if (n > (max - min + 1) || max < min) {
            return;
        }
        for (int i = 0; i < n; i++) {
            int num = (int) (Math.random() * (max - min)) + min;
            set.add(num);
        }
        int setSize = set.size();
        if (setSize < n) {
            randomSet(min, max, n - setSize, set);
        }
    }
    public static void trySleep(long time) {
        try {
            Thread.sleep(time);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    private static boolean tryRest() {
        if (mRest) {
            trySleep(mRestTime);
            return true;
        }
        return false;
    }
    public static void startAllThread(List<Thread> list) {
        for (Thread s : list) {
            s.start();
            trySleep(new Random().nextInt(2));
        }
    }
    public static void waitAllThreadFinish(List<Thread> list) {
        for (Thread s : list) {
            try {
                s.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } finally {
                if (s == null)
                    return;
                if (s.isAlive()) {
                    synchronized (list) {
                        list.remove(s);
                    }
                }
            }
        }
    }
    public static void testCase23() {
        resetTestEnvirment(mAllThreadReference);
        ArrayList<Thread> list = new ArrayList<Thread>();
        Thread t = null;
        for (int i = 0; i < DEFAULT_THREAD_NUM; i++) {
            int slot = new Random().nextInt(MAX_SLOT_NUM);
            int size = new Random().nextInt(ALLOC_12K) + ALLOC_4K;
            MemAllocMethod allocMethod = new ContinueRandomAllocMethod(size, ENUM_RANGE.ENUM_RANGE_ALL, 6, (int) Math.random() * 100, Math.random() > 0.5d, 100, 400, 0, false);
            t = new Thread(new MemAlloc(size, slot, mAllThreadReference, i, MemAlloc.SAVE_ALL, true, 1000, DEFAULT_REPEAT_TIMES, null, allocMethod), "testCase23_alloc_" + (i + 1));
            list.add(t);
        }
        startAllThread(list);
        waitAllThreadFinish(list);
    }
    public static void main(String[] args) {
        testCase23();
        Runtime.getRuntime().gc();
        testCase23();
        System.out.println("ExpectResult");
    }
    public static int makesureprop(int prop) {
        if (prop > 100 || prop <= 0) {
            prop = 100;
        }
        return prop;
    }
    public static ArrayList<Integer> choose_idx(int min, int max, int chosen, boolean random) {
        if (min > max) {
            return null;
        }
        ArrayList<Integer> ret = new ArrayList<Integer>();
        if (!random) {
            int grow = (max - min + chosen) / chosen;
            for (int i = min; i < max; i = i + grow) {
                Integer a = Integer.valueOf(i);
                ret.add(a);
            }
        } else {
            Random rand = new Random();
            for (int i = 0; i < chosen; i++) {
                int randNumber = rand.nextInt(max - min) + min;
                Integer rdn = Integer.valueOf(randNumber);
                ret.add(rdn);
            }
        }
        return ret;
    }
    public static int idx_to_size(int idx) {
        int ret = 0;
        if (idx < 0 || idx > GLOBAL_MAX_IDX) {
            return 112;
        }
        if (idx < GLOBAL_MAX_IDX) {
            ret = (idx + 1) * 8 + OBJ_HEADSIZE;
        } else {
            // idx equals GLOBAL_MAX_IDX
            ret = OBJ_MAX_SIZE + OBJ_HEADSIZE;
        }
        return ret;
    }
    public static long pagesize_to_allocsize(long size_) {
        long ret = 0;
        int pagenum = 0;
        if (size_ > (PAGE_SIZE + PAGE_HEADSIZE)) {
            pagenum = (int) (size_ / (PAGE_SIZE + PAGE_HEADSIZE));
            size_ = size_ - pagenum * (PAGE_SIZE + PAGE_HEADSIZE);
        }
        if (size_ < PAGE_SIZE) {
            ret = pagenum * PAGE_SIZE + size_;
        } else {
            ret = pagenum * PAGE_SIZE + PAGE_SIZE;
        }
        return ret;
    }
    public static ArrayList<AllocUnit> alloc_byte_idx(long bytesize, int idx, int diff, boolean isStress) {
        ArrayList<AllocUnit> array = new ArrayList<AllocUnit>();
        if (idx < LOCAL_MIN_IDX || idx > GLOBAL_MAX_IDX) {
            return null;
        }
        bytesize = pagesize_to_allocsize(bytesize);
        int size_ = idx_to_size(idx);
        if (size_ <= 0) {
            return null;
        }
        int sizenum = (int) (bytesize / size_);
        if (sizenum == 0) {
            return null;
        }
        if (diff >= 0 || (sizenum + diff) > 0) {
            sizenum = sizenum + diff;
        }
        for (int j = 0; j < sizenum; j++) {
            array.add(MemAlloc.AllocOneUnit(size_, isStress));
        }
        return array;
    }
    public static ArrayList<AllocUnit> alloc_choose_prop(ArrayList<Integer> purpose, ArrayList<Integer> other, int prop, long size, int diff, boolean isStress) {
        prop = makesureprop(prop);
        ArrayList<AllocUnit> array = new ArrayList<AllocUnit>();
        long size_alloc = 0;
        int n_p = prop / 10, n_o = (100 - prop) / 10;
        boolean no_alloc = true;
        while (size_alloc < size) {
            for (int i = 0; i < n_p; i++) {
                Iterator<Integer> it1 = purpose.iterator();
                while (it1.hasNext()) {
                    int a = (it1.next()).intValue();
                    size_alloc = size_alloc + idx_to_size(a);
                    ArrayList<AllocUnit> temp_array = alloc_byte_idx(idx_to_size(a), a, diff, isStress);
                    array.addAll(temp_array);
                    no_alloc = false;
                }
                if (size_alloc > size) {
                    return array;
                }
            }
            for (int i = 0; i < n_o; i++) {
                Iterator<Integer> it2 = other.iterator();
                while (it2.hasNext()) {
                    int a = (it2.next()).intValue();
                    size_alloc = size_alloc + idx_to_size(a);
                    ArrayList<AllocUnit> temp_array = alloc_byte_idx(idx_to_size(a), a, diff, isStress);
                    array.addAll(temp_array);
                    no_alloc = false;
                }
                if (size_alloc > size) {
                    return array;
                }
            }
            if (no_alloc)
                return array;
        }
        return array;
    }
    public static ArrayList<AllocUnit> alloc_choose_idx(long size, ENUM_RANGE range, int chosen, int prop, boolean random, boolean isStress) {
        ArrayList<Integer> purpose = new ArrayList<Integer>();
        ArrayList<Integer> other = new ArrayList<Integer>();
        ArrayList<Integer> largeobj_list = new ArrayList<Integer>();
        ArrayList<AllocUnit> array = new ArrayList<AllocUnit>();
        boolean has_small = false, has_large = false;
        if (!random) {
            switch (range) {
                case ENUM_RANGE_LOCAL:
                    purpose.addAll(choose_idx(LOCAL_MIN_IDX, GLOBAL_MIN_IDX, chosen, random));
                    has_small = true;
                    break;
                case ENUM_RANGE_GLOBAL:
                    purpose.addAll(choose_idx(GLOBAL_MIN_IDX, GLOBAL_MAX_IDX + 1, chosen, random));
                    has_small = true;
                    break;
                case ENUM_RANGE_BOTH:
                    chosen = chosen / 2;
                    purpose.addAll(choose_idx(LOCAL_MIN_IDX, GLOBAL_MIN_IDX, chosen, random));
                    other.addAll(choose_idx(GLOBAL_MIN_IDX, GLOBAL_MAX_IDX + 1, chosen, random));
                    has_small = true;
                    break;
                case ENUM_RANGE_LARGEOBJ:
                    largeobj_list.add(LARGE_PAGE_SIZE);
                    largeobj_list.add(8 * LARGE_PAGE_SIZE);
                    has_large = true;
                    break;
                case ENUM_RANGE_ALL:
                    chosen = chosen / 2;
                    purpose.addAll(choose_idx(LOCAL_MIN_IDX, GLOBAL_MIN_IDX, chosen, random));
                    other.addAll(choose_idx(GLOBAL_MIN_IDX, GLOBAL_MAX_IDX + 1, chosen, random));
                    largeobj_list.add(LARGE_PAGE_SIZE);
                    largeobj_list.add(8 * LARGE_PAGE_SIZE);
                    has_small = true;
                    has_large = true;
                    break;
                default:
                    break;
            }
        } else {
            switch (range) {
                case ENUM_RANGE_LOCAL:
                    purpose.addAll(choose_idx(LOCAL_MIN_IDX, LOCAL_MAX_IDX, chosen, random));
                    has_small = true;
                    break;
                case ENUM_RANGE_GLOBAL:
                    purpose.addAll(choose_idx(GLOBAL_MIN_IDX, GLOBAL_MAX_IDX, chosen, random));
                    has_small = true;
                    break;
                case ENUM_RANGE_BOTH:
                    chosen = chosen / 2;
                    purpose.addAll(choose_idx(LOCAL_MIN_IDX, LOCAL_MAX_IDX, chosen, random));
                    other.addAll(choose_idx(GLOBAL_MIN_IDX, GLOBAL_MAX_IDX, chosen, random));
                    has_small = true;
                    break;
                case ENUM_RANGE_LARGEOBJ:
                    Random rand = new Random();
                    int largenum = LARGEOBJ_CHOSEN_NUM;
                    for (int i = 0; i < largenum; i++) {
                        int randNumber = rand.nextInt(32) + 1;
                        largeobj_list.add(randNumber * LARGE_PAGE_SIZE);
                    }
                    has_large = true;
                    break;
                case ENUM_RANGE_ALL:
                    chosen = chosen / 2;
                    purpose.addAll(choose_idx(LOCAL_MIN_IDX, LOCAL_MAX_IDX, chosen, random));
                    other.addAll(choose_idx(GLOBAL_MIN_IDX, GLOBAL_MAX_IDX, chosen, random));
                    Random rand2 = new Random();
                    int largenum2 = LARGEOBJ_CHOSEN_NUM;
                    for (int i = 0; i < largenum2; i++) {
                        int randNumber = rand2.nextInt(32) + 1;
                        largeobj_list.add(randNumber * LARGE_PAGE_SIZE);
                    }
                    has_large = true;
                    has_small = true;
                    break;
                default:
            }
        }
        if (has_small) {
            array.addAll(alloc_choose_prop(purpose, other, prop, size, 0, isStress));
        }
        if (has_large) {
            array.addAll(alloc_list_large(LARGEOBJ_ALLOC_SIZE, largeobj_list, 50, isStress));
        }
        return array;
    }
    public static ArrayList<AllocUnit> alloc_list_large(long bytesize, ArrayList<Integer> alloc_list, int prop, boolean isStress) {
        ArrayList<AllocUnit> array = new ArrayList<AllocUnit>();
        long size_allocated = 0;
        prop = makesureprop(prop);
        int n_p = prop / 10, n_o = (100 - prop) / 10;
        while (size_allocated < bytesize) {
            Iterator<Integer> it1 = alloc_list.iterator();
            boolean first = true;
            int loopn = n_p;
            while (it1.hasNext()) {
                int a = (it1.next()).intValue();
                if (first) {
                    loopn = n_p;
                    first = false;
                } else {
                    loopn = n_o;
                    first = true;
                }
                for (int i = 0; i < loopn; i++) {
                    size_allocated = size_allocated + a;
                    ArrayList<AllocUnit> temp_array = alloc_byte_large(a, a, 0, isStress);
                    array.addAll(temp_array);
                    if (size_allocated > bytesize) {
                        return array;
                    }
                }
            }
        }
        return array;
    }
    public static ArrayList<AllocUnit> alloc_byte_large(long bytesize, int alloc_size, int diff, boolean isStress) {
        ArrayList<AllocUnit> array = new ArrayList<AllocUnit>();
        if (alloc_size <= OBJ_MAX_SIZE) {
            return null;
        }
        long size = 0;
        if ((bytesize + diff * alloc_size) <= 0) {
            return null;
        }
        bytesize = bytesize + diff * alloc_size;
        while (size < bytesize) {
            size = size + alloc_size;
            array.add(MemAlloc.AllocOneUnit(alloc_size, isStress));
        }
        return array;
    }
    public static class RandomAllocMethod extends MemAllocMethod {
        public RandomAllocMethod(long size, ENUM_RANGE range, int chosen, int prop, boolean random) {
            mAllocSize = size;
            mRange = range;
            mChosen = chosen;
            mProp = prop;
            mRandorm = random;
        }
        @Override
        public List<AllocUnit> alloc(boolean isStress) {
            List<AllocUnit> ret = alloc_choose_idx(mAllocSize, mRange, mChosen, mProp, mRandorm, isStress);
            return ret;
        }
    }
    static class FreeReference implements Runnable {
        private List<List<AllocUnit>> mAllReference = null;
        private int mThreadIdx = 0;
        private boolean mLoop = false;
        private boolean mSync = false;
        public FreeReference(List<List<AllocUnit>> reference, int threadIdx, boolean loop, boolean sync) {
            mAllReference = reference;
            mThreadIdx = threadIdx;
            mLoop = loop;
            mSync = sync;
        }
        public FreeReference(List<List<AllocUnit>> reference, int threadIdx, boolean loop) {
            this(reference, threadIdx, loop, false);
        }
        private void release() {
            if (mSync) {
                synchronized (mAllReference) {
                    releaseOperate();
                }
            } else {
                releaseOperate();
            }
        }
        private void releaseOperate() {
            if (mAllReference == null) {
                return;
            }
            if (mThreadIdx >= mAllReference.size()) {
                return;
            }
            List<AllocUnit> cur = mAllReference.get(mThreadIdx);
            mAllReference.set(mThreadIdx, null);
            if (cur != null) {
                cur.clear();
            }
        }
        @Override
        public void run() {
            if (!mLoop) {
                release();
                return;
            }
            while (mLoop) {
                if (!mRunning) {
                    break;
                }
                release();
                trySleep(1);
            }
        }
    }
    public static class ContinueRandomAllocMethod extends MemAllocMethod {
        private long mSleep;
        private long mTotalTimes;
        private long mTimes = 0;
        private boolean mFreeImmediately;
        public ContinueRandomAllocMethod(long size, ENUM_RANGE range, int chosen, int prop, boolean random, long sleep, long totalTimes, int times, boolean freeImmediately) {
            mAllocSize = size;
            mRange = range;
            mChosen = chosen;
            mProp = prop;
            mRandorm = random;
            mSleep = sleep;
            mTotalTimes = totalTimes;
            mTimes = times;
            mFreeImmediately = freeImmediately;
        }
        @Override
        public List<AllocUnit> alloc(boolean isStress) {
            List<AllocUnit> ret = null;
            List<AllocUnit> tmp = null;
            if (mTotalTimes == 0 && mTimes == 0) {
                ret = alloc_choose_idx(mAllocSize, mRange, mChosen, mProp, mRandorm, isStress);
                return ret;
            }
            long elipseTime = 0;
            boolean find = false;
            ret = new ArrayList<AllocUnit>();
            while (elipseTime < mTotalTimes) {
                long now = System.currentTimeMillis();
                tmp = alloc_choose_idx(mAllocSize, mRange, mChosen, mProp, mRandorm, isStress);
                if (!mFreeImmediately && tmp != null) {
                    ret.addAll(tmp);
                } else {
                    tmp = null;
                }
                find = true;
                trySleep(mSleep);
                elipseTime += System.currentTimeMillis() - now;
            }
            if (!find) {
                int time = 0;
                while (time < mTotalTimes) {
                    tmp = alloc_choose_idx(mAllocSize, mRange, mChosen, mProp, mRandorm, isStress);
                    if (!mFreeImmediately && tmp != null) {
                        ret.addAll(tmp);
                    } else {
                        tmp = null;
                    }
                    time++;
                    trySleep(mSleep);
                }
            }
            return ret;
        }
    }
    public static class AllocUnit {
        public byte unit[];
        public AllocUnit(int arrayLength) {
            unit = new byte[arrayLength];
        }
    }
    public static enum ENUM_RANGE {
        ENUM_RANGE_NONE, ENUM_RANGE_LOCAL, ENUM_RANGE_GLOBAL, ENUM_RANGE_BOTH, ENUM_RANGE_LARGEOBJ, ENUM_RANGE_ALL
    }
    public static abstract class MemAllocMethod {
        protected long mAllocSize;
        protected ENUM_RANGE mRange;
        protected int mChosen = 0;
        protected int mProp = 0;
        protected boolean mRandorm;
        public abstract List<AllocUnit> alloc(boolean isStress);
    }
    public static class AllocUnitWithFinalizeSleep extends AllocUnit {
        public AllocUnitWithFinalizeSleep(int arrayLength) {
            super(arrayLength);
        }
        @Override
        public void finalize() throws Throwable {
            trySleep(1000);
        }
    }
    public static class AllocUnitWithSoftReference extends AllocUnit {
        private Reference<byte[]> refArray;
        public AllocUnitWithSoftReference(int arrayLength) {
            super(arrayLength);
            refArray = new SoftReference<byte[]>(new byte[arrayLength]);
        }
    }
    public static class AllocUnitWithFinalize extends AllocUnit {
        public AllocUnitWithFinalize(int arrayLength) {
            super(arrayLength);
        }
        @Override
        public void finalize() throws Throwable {
            unit = null;
        }
    }
    public static class AllocUnitCycleMaster extends AllocUnit {
        @SuppressWarnings("unused")
        private AllocUnitCycleSlave slave = null;
        public AllocUnitCycleMaster(int arrayLength) {
            super(arrayLength);
            slave = new AllocUnitCycleSlave(arrayLength, this);
        }
    }
    public static class AllocUnitCycleSlave extends AllocUnit {
        private AllocUnitCycleMaster master = null;
        public AllocUnitCycleSlave(int arrayLength, AllocUnitCycleMaster master) {
            super(arrayLength);
            this.master = master;
        }
    }
    public static class AllocUnitwithCleaner extends AllocUnit {
        public AllocUnitwithCleaner(int arrayLength) {
            super(arrayLength);
            this.cleaner = Cleaner.create(this, new TestCleaner(new byte[arrayLength]));
        }
        private final Cleaner cleaner;
        private static class TestCleaner implements Runnable {
            public byte unit[];
            public TestCleaner(byte unit[]) {
                this.unit = unit;
            }
            public void run() {
                this.unit = null;
            }
        }
    }
    public static class AllocUnitWithWeakRefrence extends AllocUnit {
        public WeakReference<byte[]> weakIntArray;
        public AllocUnitWithWeakRefrence(int arrayLength) {
            super(arrayLength);
            weakIntArray = new WeakReference<byte[]>(new byte[arrayLength]);
        }
    }
    public static class AllocUnitWithRefAndCleanerF extends AllocUnit {
        private Reference<byte[]> refArray;
        private Cleaner cleaner;
        public AllocUnitWithRefAndCleanerF(int arrayLength) {
            super(arrayLength);
            refArray = new SoftReference<byte[]>(new byte[arrayLength]);
            cleaner = Cleaner.create(this, new TestCleaner(new byte[arrayLength]));
        }
        private static class TestCleaner implements Runnable {
            public byte[] unit;
            public TestCleaner(byte[] unit) {
                this.unit = unit;
            }
            public void run() {
                this.unit = null;
            }
        }
        @Override
        protected void finalize() throws Throwable {
            unit = null;
        }
    }
    public static class AllocUnitWithWeakRefAndCleanerF extends AllocUnit {
        private WeakReference<byte[]> weakRefArray;
        private Cleaner cleaner;
        public AllocUnitWithWeakRefAndCleanerF(int arrayLength) {
            super(arrayLength);
            weakRefArray = new WeakReference<byte[]>(new byte[arrayLength]);
            cleaner = Cleaner.create(this, new TestCleaner(new byte[arrayLength]));
        }
        private static class TestCleaner implements Runnable {
            public byte[] unit;
            public TestCleaner(byte[] unit) {
                this.unit = unit;
            }
            public void run() {
                this.unit = null;
            }
        }
        @Override
        protected void finalize() throws Throwable {
            unit = null;
        }
    }
    public static class ReAlive {
        public static ArrayList<AllocUnitWithRefAndWeakRefAndCleanerActiveObjF.TestCleaner> cleanerList
                = new ArrayList<AllocUnitWithRefAndWeakRefAndCleanerActiveObjF.TestCleaner>(MAX_REALIVE_OBJ_NUM);
        public static ArrayList<AllocUnitWithRefAndWeakRefAndCleanerFActiveObj> refAndWeakRefAndCleanerFActiveObjList
                = new ArrayList<AllocUnitWithRefAndWeakRefAndCleanerFActiveObj>(MAX_REALIVE_OBJ_NUM);
    }
    public static class AllocUnitWithRefAndWeakRefAndCleanerActiveObjF extends AllocUnit {
        private Reference<byte[]> refArray;
        private WeakReference<byte[]> weakRefArray;
        private Cleaner cleaner;
        public AllocUnitWithRefAndWeakRefAndCleanerActiveObjF(int arrayLength) {
            super(arrayLength);
            refArray = new SoftReference<byte[]>(new byte[arrayLength]);
            weakRefArray = new WeakReference<byte[]>(new byte[arrayLength]);
            cleaner = Cleaner.create(this, new TestCleaner(new byte[arrayLength]));
        }
        private static class TestCleaner implements Runnable {
            public byte[] unit;
            public TestCleaner(byte[] unit) {
                this.unit = unit;
            }
            public void run() {
                if (ReAlive.cleanerList.size() > MAX_REALIVE_OBJ_NUM) {
                    ReAlive.cleanerList.clear();
                } else {
                    ReAlive.cleanerList.add(this);
                }
                this.unit = null;
            }
        }
        @Override
        protected void finalize() throws Throwable {
            unit = null;
        }
    }
    public static class AllocUnitWithRefAndWeakRefAndCleanerFActiveObj extends AllocUnit {
        private Reference<byte[]> refArray;
        private WeakReference<byte[]> weakRefArray;
        private Cleaner cleaner;
        public AllocUnitWithRefAndWeakRefAndCleanerFActiveObj(int arrayLength) {
            super(arrayLength);
            refArray = new SoftReference<byte[]>(new byte[arrayLength]);
            weakRefArray = new WeakReference<byte[]>(new byte[arrayLength]);
            cleaner = Cleaner.create(this, new TestCleaner(new byte[arrayLength]));
        }
        private static class TestCleaner implements Runnable {
            public byte[] unit;
            public TestCleaner(byte[] unit) {
                this.unit = unit;
            }
            public void run() {
                this.unit = null;
            }
        }
    }
    static class MemAlloc implements Runnable {
        int mAllocSize = 4 * 1024;
        int mSaveNum = -1;
        boolean mFreeImmediately = false;
        int mSlot = 0;
        int mSleepTime = 5;
        List<List<AllocUnit>> mSaveTo = null;
        int mSaveIndex = 0;
        public static final int LOCAL_MIN_IDX = 0;
        public static final int GLOBAL_MAX_IDX = 62;
        static final int PAGE_SIZE = 4016;
        static final int PAGE_HEADSIZE = 80;
        static final int OBJ_MAX_SIZE = 1008;
        static final int OBJ_HEADSIZE = 16;
        public static final int SAVE_ALL = -1;
        public static final int SAVE_HALF = -2;
        public static final int SAVE_HALF_RAMDOM = -3;
        public static final int SAVE_HALF_MINUS = -4;
        private int mRepeatTimes = 1;
        public static volatile int totalAllocedNum = 0;
        public static boolean TEST_OOM_FINALIZER = false;
        private Runnable mCallBackAfterFree = null;
        private MemAllocMethod mAllocMethod = null;
        private boolean mIsStress;
        public MemAlloc(boolean isStress, int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime, int repeatTimes, Runnable callBackAfterFree, MemAllocMethod allMethod) {
            mIsStress = isStress;
            mAllocSize = allocSize;
            mFreeImmediately = freeImmediately;
            mSaveNum = saveNum;
            mSlot = slot;
            mSleepTime = sleepTime;
            mSaveTo = saveTo;
            mSaveIndex = saveIndex;
            mRepeatTimes = repeatTimes;
            mCallBackAfterFree = callBackAfterFree;
            mAllocMethod = allMethod;
        }
        public MemAlloc(int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime, int repeatTimes, Runnable callBackAfterFree, MemAllocMethod allMethod) {
            this(false, allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, repeatTimes, callBackAfterFree, null);
        }
        public MemAlloc(int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime, int repeatTimes, Runnable callBackAfterFree) {
            this(allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, repeatTimes, callBackAfterFree, null);
        }
        public MemAlloc(int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime, int repeatTimes) {
            this(allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, repeatTimes, null);
        }
        /**
         * saveNum:0, no obj will be reference by gloal, -1:all will be
         * reference.
         **/
        public MemAlloc(int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime) {
            this(allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, 1);
        }
        public MemAlloc(boolean isStress, int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime, int repeatTimes, Runnable callBackAfterFree) {
            this(isStress, allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, repeatTimes, callBackAfterFree, null);
        }
        public MemAlloc(boolean isStress, int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime, int repeatTimes) {
            this(isStress, allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, repeatTimes, null);
        }
        public MemAlloc(boolean isStress, int allocSize, int slot, List<List<AllocUnit>> saveTo, int saveIndex, int saveNum, boolean freeImmediately, int sleepTime) {
            this(isStress, allocSize, slot, saveTo, saveIndex, saveNum, freeImmediately, sleepTime, 1);
        }
        private List<AllocUnit> merge(List<AllocUnit> src1, List<AllocUnit> src2) {
            if (src1 == null && src2 == null) {
                return null;
            }
            List<AllocUnit> list = new ArrayList<AllocUnit>();
            if (src1 != null) {
                list.addAll(src1);
                src1.clear();
            }
            if (src2 != null) {
                list.addAll(src2);
                src2.clear();
            }
            return list;
        }
        public static long pagesize_to_allocsize(long size_) {
            long ret = 0;
            int pagenum = 0;
            if (size_ > (PAGE_SIZE + PAGE_HEADSIZE)) {
                pagenum = (int) (size_ / (PAGE_SIZE + PAGE_HEADSIZE));
                size_ = size_ - pagenum * (PAGE_SIZE + PAGE_HEADSIZE);
            }
            if (size_ < PAGE_SIZE) {
                ret = pagenum * PAGE_SIZE + size_;
            } else {
                ret = pagenum * PAGE_SIZE + PAGE_SIZE;
            }
            return ret;
        }
        public static int idx_to_size(int idx) {
            int ret = 0;
            if (idx < 0 || idx > GLOBAL_MAX_IDX) {
                return 112;
            }
            if (idx < GLOBAL_MAX_IDX) {
                ret = (idx + 1) * 8 + OBJ_HEADSIZE;
            } else {
                // idx equals GLOBAL_MAX_IDX
                ret = OBJ_MAX_SIZE + OBJ_HEADSIZE;
            }
            return ret;
        }
        public static AllocUnit AllocOneUnit(int alloc_size, boolean isStress) {
            if (TEST_OOM_FINALIZER) {
                return new AllocUnitWithFinalizeSleep(alloc_size);
            }
            int curNum;
            totalAllocedNum++;
            curNum = totalAllocedNum % 100;
            if (!isStress) {
                if (curNum < 1) {
                    return new AllocUnitWithFinalize(alloc_size);
                }
                if (curNum < 2) {
                    return new AllocUnitCycleMaster(alloc_size);
                }
                if (curNum < 3) {
                    return new AllocUnitwithCleaner(alloc_size);
                }
                if (curNum < 4) {
                    return new AllocUnitWithWeakRefrence(alloc_size);
                }
                if (curNum < 5) {
                    return new AllocUnitWithRefAndCleanerF(alloc_size);
                }
                if (curNum < 6) {
                    return new AllocUnitWithWeakRefAndCleanerF(alloc_size);
                }
                if (curNum < 7) {
                    return new AllocUnitWithRefAndWeakRefAndCleanerActiveObjF(alloc_size);
                }
                if (curNum < 8) {
                    return new AllocUnitWithRefAndWeakRefAndCleanerFActiveObj(alloc_size);
                }
            } else {
                if (curNum < 1) {
                    return new AllocUnitCycleMaster(alloc_size);
                }
                if (curNum < 2) {
                    return new AllocUnitWithWeakRefrence(alloc_size);
                }
                if (curNum < 3) {
                    return new AllocUnitWithSoftReference(alloc_size);
                }
            }
            return new AllocUnit(alloc_size);
        }
        public static ArrayList<AllocUnit> alloc_byte_idx(long bytesize, int idx, int diff, boolean isStress) {
            ArrayList<AllocUnit> array = new ArrayList<AllocUnit>();
            if (idx < LOCAL_MIN_IDX || idx > GLOBAL_MAX_IDX) {
                return null;
            }
            bytesize = pagesize_to_allocsize(bytesize);
            int size_ = idx_to_size(idx);
            if (size_ <= 0) {
                return null;
            }
            int sizenum = (int) (bytesize / size_);
            if (sizenum == 0) {
                return null;
            }
            if (diff >= 0 || (sizenum + diff) > 0) {
                sizenum = sizenum + diff;
            }
            for (int j = 0; j < sizenum; j++) {
                array.add(MemAlloc.AllocOneUnit(size_, isStress));
            }
            return array;
        }
        @Override
        public void run() {
            for (int j = 0; j < mRepeatTimes || mRepeatTimes < 0; j++) {
                if (!mRunning) {
                    break;
                }
                tryRest();
                List<AllocUnit> ret = null;
                if (mAllocMethod == null) {
                    ret = alloc_byte_idx(mAllocSize, mSlot, 0, mIsStress);
                } else {
                    ret = mAllocMethod.alloc(mIsStress);
                }
                if (ret == null) {
                    // Log.e(TAG,
                    // "Fatal error happen when alloc memory:  alloc size: "+mAllocSize+" slot:"+mSlot);
                    continue;
                } else {
                    // Log.e(TAG,
                    // "alloc memory sucessfull:  alloc size: "+mAllocSize+" slot:"+mSlot);
                }
                trySleep(mSleepTime);
                if (mFreeImmediately) {
                    if (ret != null) {
                        ret.clear();
                        ret = null;
                    }
                    if (mCallBackAfterFree != null) {
                        mCallBackAfterFree.run();
                    }
                    continue;
                }
                if (mSaveTo != null && mSaveNum != 0) {
                    if (mSaveNum == SAVE_ALL && mSaveIndex < mSaveTo.size()) {
                    } else if (mSaveNum == SAVE_HALF && mSaveIndex < mSaveTo.size() && ret != null) {
                        int half = ret.size() / 2;
                        for (int i = ret.size() - 1; i > half; i--) {
                            ret.remove(i);
                        }
                    } else if (mSaveNum == SAVE_HALF_MINUS && mSaveIndex < mSaveTo.size() && ret != null) {
                        int half = ret.size() / 2 - 1;
                        half = half < 0 ? 0 : half;
                        for (int i = ret.size() - 1; i > half; i--) {
                            ret.remove(i);
                        }
                    } else if (mSaveNum == SAVE_HALF_RAMDOM && mSaveIndex < mSaveTo.size() && ret != null) {
                        Set<Integer> free = getRandomIndex(ret, ret.size() / 2 + 1);
                        for (Integer index : free) {
                            ret.set(index, null);
                        }
                    } else if (ret != null) {
                        int i = ret.size() > mSaveNum ? mSaveNum : ret.size();
                        for (i = ret.size() - 1; i > 0; i--) {
                            ret.remove(i);
                        }
                    }
                    if (mCallBackAfterFree != null) {
                        mCallBackAfterFree.run();
                    }
                    synchronized (mSaveTo) {
                        mSaveTo.set(mSaveIndex, ret);
                    }
                }
                trySleep(mSleepTime);
            }
        }
    }
}
