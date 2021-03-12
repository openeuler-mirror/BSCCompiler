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

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
public class TrimTest10 {
    private static final int MEMORY_LIMIT = 100;
    private static ArrayList<byte[]> store = new ArrayList<byte[]>();
    public static void main(String[] args) throws Exception {
        Thread.sleep(2000);
        for (int i = 0; i < MEMORY_LIMIT; i++) {
            byte[] temp = new byte[1024 * 1024];
            for (int j = 0; j < 1024 * 1024; j += 4096) {
                temp[j] = (byte) 1;
            }
            store.add(temp);
        }
        int pid = new TrimTest10().getPid();
        String command = "showmap " + pid;
        Process process = Runtime.getRuntime().exec(command);
        BufferedReader input = new BufferedReader(new InputStreamReader(process.getInputStream()));
        String rosInfo = "";
        while ((rosInfo = input.readLine()) != null) {
            if (rosInfo.contains("maple_alloc_ros]")) {
                rosInfo = rosInfo.split("\\s+")[3];
                break;
            }
        }
        input.close();
        store.clear();
        store = null;
        Thread.sleep(4000);
        Runtime.getRuntime().gc();
        Thread.sleep(3000);
        process = Runtime.getRuntime().exec(command);
        input = new BufferedReader(new InputStreamReader(process.getInputStream()));
        String rosInfo2 = "";
        while ((rosInfo2 = input.readLine()) != null) {
            if (rosInfo2.contains("maple_alloc_ros]")) {
                rosInfo2 = rosInfo2.split("\\s+")[3];
                break;
            }
        }
        input.close();
        if (Integer.valueOf(rosInfo) > MEMORY_LIMIT * 1024 && Integer.valueOf(rosInfo2) < 5 * 1024) {
            System.out.println("ExpectResult");
        }
    }
    public native int getPid();
}
