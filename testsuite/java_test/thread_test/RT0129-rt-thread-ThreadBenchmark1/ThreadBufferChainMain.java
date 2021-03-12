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


import java.io.IOException;
import java.util.ArrayList;
/**
 * Thread buffer synchronization benchmark.
 * @author Christoph Knabe, {@linkplain http://public.beuth-hochschule.de/~knabe/ }
 * @since 2016-04-15
*/

public class ThreadBufferChainMain {
    public static void main(final String[] args) throws InterruptedException, IOException {
        for(int chainLength = 10; chainLength < 200; chainLength *= 10) {
            benchmarkThreadBufferChain(chainLength);
        }
        System.out.println(0);
    }
    /**
     * Creates a Thread-Buffer chain with the given number of threads, and sends number messages through this chain.
     * Thus this method causes number*number thread switches.
     * Reports the elapsed time from start of the sending to arrival of the last message at the end of the chain.
     * @param number Number of threads in the Thread-Buffer chain
    */

    private static void benchmarkThreadBufferChain(final int number) throws InterruptedException {
        Buffer lastBuf = new Buffer();
        final Consumer consumer = new Consumer(lastBuf);
        consumer.start();
        ArrayList<Forwarder> forwarders = new ArrayList<>();
        for(int i = 3; i <= number; i++) {
            final Buffer newBuf = new Buffer();
            final Forwarder forwarder = new Forwarder(newBuf, lastBuf);
            forwarder.start();
            lastBuf = newBuf;
            forwarders.add(forwarder);
        }
        final Producer producer = new Producer(lastBuf, number);
        producer.start();
        consumer.join();
        try{
            producer.join();
            for (int i = 0; i < forwarders.size(); i++) {
                forwarders.get(i).interrupt();
            }
        } catch (Exception e) {
            System.out.println(e);
        }
    }
}
class Buffer {
    private int value;
    private boolean available = false;
    public synchronized int get() throws InterruptedException {
        while (!available) {
            wait();
        }
        available = false;
        notifyAll();
        return value;
    }
    public synchronized void put(final int value) throws InterruptedException {
        while (available) {
            wait();
        }
        this.value = value;
        available = true;
        notifyAll();
    }
}
class Consumer extends Thread {
    private final Buffer buffer;
    public Consumer(final Buffer buffer) {
        this.buffer = buffer;
    }
    public void run() {
        try {
            for (;;) {
                final int value = buffer.get();
                if(value<0) {
                    break;
                }
            }
        } catch (InterruptedException e) {
            System.out.println(e);
        }
    }
}
/**
 * The Forwarder is the most important type of the inner nodes of the Thread chain.
 * The message passing between the Producer and the first Forwarder, between two adjacent Forwarders,
 * and between the last Forwarder and the Consumer takes place by using a synchronized Buffer
 * according to the Java Tutorial, Trail Essential Java Classes,
 * Lesson Threads: Doing Two or More Tasks At Once, The Producer/Consumer Example.
 * Findable for example at {@linkplain http://www.eng.nene.ac.uk/~gary/javatut/essential/threads/synchronization.html}
*/

class Forwarder extends Thread {
    private final Buffer inputBuffer;
    private final Buffer outputBuffer;
    public Forwarder(final Buffer inputBuffer, final Buffer outputBuffer) {
        this.inputBuffer = inputBuffer;
        this.outputBuffer = outputBuffer;
    }
    public void run() {
        try {
            for (;;) {
                final int value = inputBuffer.get();
                outputBuffer.put(value);
            }
        } catch (InterruptedException e) {
            //ignore the interrupted exception
        }
    }
}
class Producer extends Thread {
    private final Buffer buffer;
    private final int numberOfItems;
    public Producer(final Buffer cubbyHole, final int numberOfItems) {
        this.buffer = cubbyHole;
        this.numberOfItems = numberOfItems;
    }
    public void run() {
        try {
            for (int i = 0; i <= numberOfItems; i++) {
                final int msg = i<numberOfItems ? i : -1;
                buffer.put(msg);
            }
        } catch (InterruptedException e) {
            System.out.println(e);
        }
    }
}