/*
 *- @TestCaseID:rc/stress/Memory_normalTestCase6.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:
 *- @Priority: High
 *- @Source: Memory_normalTestCase6.java
 *- @ExecuteClass: Memory_normalTestCase6
 *- @ExecuteArgs:
 *- @Remark:
 *- @Author:从压力测试case中拆解 liuweiqing l00481345
 */


import sun.misc.Cleaner;

import java.lang.ref.Reference;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

public class Memory_normalTestCase6 {
    private static final int DEFAULT_THREAD_NUM = 80;
    static final int MAX_SLOT_NUM = 62;
    private static boolean mRest = false;
    private static int mRestTime = 5000;
    static boolean mRunning = true;
    private static final int DEFAULT_REPEAT_TIMES = 1;
    private static final int ALLOC_16K = 16 * 1024;
    private static final int ALLOC_12K = 12 * 1024;
    private static final int ALLOC_8K = 8 * 1024;
    private static final int ALLOC_4K = 4 * 1024;
    private static final int ALLOC_2K = 2 * 1024;
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

    public static void testCase6() {
        resetTestEnvirment(mAllThreadReference);
        ArrayList<Thread> list = new ArrayList<Thread>();
        Thread t = null;
        t = new Thread(new MemAlloc(ALLOC_4K, (int) (Math.random() * MAX_SLOT_NUM), mAllThreadReference, 0, 1, false, 1, DEFAULT_THREAD_NUM), "testCase6_" + (1));
        list.add(t);
        startAllThread(list);
        waitAllThreadFinish(list);
        list.clear();
        for (int i = 1; i < DEFAULT_THREAD_NUM + 1; i++) {
            int slot = i % MAX_SLOT_NUM;
            t = new Thread(new MemAlloc(ALLOC_8K - getSlotSize(slot), slot, mAllThreadReference, i, MemAlloc.SAVE_ALL, false, 1), "testCase6_" + (i + 1));
            list.add(t);
        }
        startAllThread(list);
        waitAllThreadFinish(list);
        resetTestEnvirment(mAllThreadReference);
    }

    public static void main(String[] args) {
        testCase6();
        Runtime.getRuntime().gc();
        testCase6();
        System.out.println("ExpectResult");
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
                array.add(AllocOneUnit(size_, isStress));
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

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
