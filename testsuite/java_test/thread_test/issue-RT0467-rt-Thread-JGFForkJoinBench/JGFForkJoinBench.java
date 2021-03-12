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


import java.util.Hashtable;
public class JGFForkJoinBench implements JGFSection1{
    public static int nThreads;
    private static final int INITSIZE = 1000;
    private static final int MAXSIZE = 10000;
    private static final double TARGETTIME = 10.0;
    public JGFForkJoinBench(int nThreads) {
        this.nThreads = nThreads;
    }
    public void JGFRun() {
        int i;
        int size;
        double time;
        double time2;
        time = 0.0;
        time2 = 0.0;
        size = INITSIZE;
        Runnable thObjects[] = new Runnable [nThreads];
        Thread th[] = new Thread [nThreads];
        JGFInstrumentor.addTimer("Section1:ForkJoin:Simple", "forkjoins");
        while (size < MAXSIZE) {
            JGFInstrumentor.resetTimer("Section1:ForkJoin:Simple");
            JGFInstrumentor.startTimer("Section1:ForkJoin:Simple");
            for (int k = 0; k < size; k++) {
                for(i = 1; i < nThreads; i++) {
                    thObjects[i] = new ForkJoinThread(i);
                    th[i] = new Thread(thObjects[i]);
                    th[i].start();
                }
                thObjects[0] = new ForkJoinThread(0);
                thObjects[0].run();
                for(i = 1; i < nThreads; i++) {
                    try {
                        th[i].join();
                    } catch (InterruptedException e) {
                        System.out.println("Join is interrupted");
                    }
                }
            }
            JGFInstrumentor.stopTimer("Section1:ForkJoin:Simple");
            time = JGFInstrumentor.readTimer("Section1:ForkJoin:Simple");
            JGFInstrumentor.addOpsToTimer("Section1:ForkJoin:Simple",(double) size);
            size *= 2;
        }
        size /= 2;
        JGFInstrumentor.addTimer("Section1:SerialForkJoin:Simple", "forkjoins");
        JGFInstrumentor.startTimer("Section1:SerialForkJoin:Simple");
        for (int k = 0; k < size; k++) {
            thObjects[0] = new ForkJoinThread(0);
            thObjects[0].run();
        }
        JGFInstrumentor.stopTimer("Section1:SerialForkJoin:Simple");
        time2 = JGFInstrumentor.readTimer("Section1:SerialForkJoin:Simple");
        JGFInstrumentor.addTimeToTimer("Section1:ForkJoin:Simple", -time2);
        JGFInstrumentor.printPerTimer("Section1:ForkJoin:Simple");
    }
    public static void main(String[] argv) {
        nThreads = 4;
        JGFInstrumentor.printHeader(1, 0, nThreads);
        JGFForkJoinBench fj = new JGFForkJoinBench(nThreads);
        fj.JGFRun();
        System.out.println(0);
    }
}
class ForkJoinThread implements Runnable {
    int id;
    long j;
    public ForkJoinThread(int id) {
        this.id = id;
    }
    public void run() {
        // do something trivial but which won't be optimised away!
        double theta=37.2, sInt, res;
        sInt = Math.sin(theta);
        res = sInt*sInt;
    }
}
interface JGFSection1 {
    public final int INITSIZE = 10000;
    public final int MAXSIZE = 1000000000;
    public final double TARGETTIME = 1.0;
    public void JGFRun();
}
class JGFInstrumentor{
    private static Hashtable timers;
    private static Hashtable data;
    static {
        timers = new Hashtable();
        data = new Hashtable();
    }
    public static synchronized void addTimer(String name) {
        if (!timers.containsKey(name)) {
            timers.put(name, new JGFTimer(name));
        }
    }
    public static synchronized void addTimer (String name, String opName) {
        if (!timers.containsKey(name)) {
            timers.put(name, new JGFTimer(name,opName));
        }
    }
    public static synchronized void addTimer (String name, String opName, int size) {
        if (!timers.containsKey(name)) {
            timers.put(name, new JGFTimer(name,opName,size));
        }
    }
    public static synchronized void startTimer(String name) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).start();
        }
    }
    public static synchronized void stopTimer(String name) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).stop();
        }
    }
    public static synchronized void addOpsToTimer(String name, double count) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).addOps(count);
        }
    }
    public static synchronized void addTimeToTimer(String name, double added_time) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).addTime(added_time);
        }
    }
    public static synchronized double readTimer(String name) {
        double time;
        if (timers.containsKey(name)) {
            time = ((JGFTimer) timers.get(name)).time;
        } else {
            time = 0.0;
        }
        return time;
    }
    public static synchronized void resetTimer(String name) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).reset();
        }
    }
    public static synchronized void printTimer(String name) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).print();
        }
    }
    public static synchronized void printPerTimer(String name) {
        if (timers.containsKey(name)) {
            ((JGFTimer) timers.get(name)).printPer();
        }
    }
    public static synchronized void storeData(String name, Object obj) {
        data.put(name,obj);
    }
    public static synchronized void retrieveData(String name, Object obj) {
        obj = data.get(name);
    }
    public static synchronized void printHeader(int section, int size, int nThreads) {
        String header;
        String base;
        header = "";
        base = "Java Grande Forum Thread Benchmark Suite - Version 1.0 - Section ";
        switch (section) {
            case 1:
                header = base + "1";
                break;
            case 2:
                switch (size) {
                    case 0:
                        header = base + "2 - Size A";
                        break;
                    case 1:
                        header = base + "2 - Size B";
                        break;
                    case 2:
                        header = base + "2 - Size C";
                        break;
                }
                break;
            case 3:
                switch (size) {
                    case 0:
                        header = base + "3 - Size A";
                        break;
                    case 1:
                        header = base + "3 - Size B";
                        break;
                }
                break;
        }
    }
}
class JGFTimer {
    public String name;
    public String opName;
    public double time;
    public double opCount;
    public long calls;
    public int size = -1;
    private long start_time;
    private boolean on;
    public JGFTimer(String name, String opName) {
        this.name = name;
        this.opName = opName;
        reset();
    }
    public JGFTimer(String name, String opName, int size) {
        this.name = name;
        this.opName = opName;
        this.size = size;
        reset();
    }
    public JGFTimer(String name) {
        this(name,"");
    }
    public void start() {
        on = true;
        start_time = System.currentTimeMillis();
    }
    public void stop() {
        time += (double) (System.currentTimeMillis()-start_time) / 1000.;
        calls++;
        on = false;
    }
    public void addOps(double count) {
        opCount += count;
    }
    public void addTime(double added_time) {
        time += added_time;
    }
    public void reset() {
        time = 0.0;
        calls = 0;
        opCount = 0;
        on = false;
    }
    public double per() {
        return opCount / time;
    }
    public void longPrint() {
    }
    public void print() {
        if (opName.equals("")) {
        } else {
            switch(size) {
                case 0:
                    break;
                case 1:
                    break;
                case 2:
                    break;
                default:
                    break;
            }
        }
    }
    public void printPer() {
        String name;
        name = this.name;
        // pad name to 40 characters
        while ( name.length() < 40 ) {
            name = name + " ";
        }
    }
}