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


import java.io.PrintStream;
import java.nio.charset.Charset;
public class StringUserCaseCharsetTest {
    private static int processResult = 99;
    public static void main(String[] argv) {
        run(argv, System.out);
    }
    public static int run(String argv[],PrintStream out){
        int result = 2/*STATUS_FAILED*/;
        try {
            StringUserCaseCharsetTest_1();
        } catch(Exception e){
            System.out.println(e);
            processResult = processResult - 10;
        }
//        System.out.println("result: " + result);
//        System.out.println("processResult:" + processResult);
        if (result == 1 && processResult == 99){
            result =0;
        }
        return result;
    }
    public static void StringUserCaseCharsetTest_1() {
        String testCaseID = "StringUserCaseCharsetTest_1";
        System.out.println("========================" + testCaseID);
        // abc   ASCII
        byte[] abc_ASCII = {(byte) 0x61, (byte) 0x62, (byte) 0x63};
        // abc   ISO88591
        byte[] abc_ISO88591 = {(byte) 0x61, (byte) 0x62, (byte) 0x63};
        // abc   UTF8
        byte[] abc_UTF8 = {(byte) 0x61, (byte) 0x62, (byte) 0x63};
        // abc   UTF16BE
        byte[] abc_UTF16BE = {(byte) 0x00, (byte) 0x61, (byte) 0x00, (byte) 0x62, (byte) 0x00, (byte) 0x63};
        // abc   UTF16LE
        byte[] abc_UTF16LE = {(byte) 0x61, (byte) 0x00, (byte) 0x62, (byte) 0x00, (byte) 0x63, (byte) 0x00};
        // abc   UTF16
        byte[] abc_UTF16 = {(byte) 0x00, (byte) 0x61, (byte) 0x00, (byte) 0x62, (byte) 0x00, (byte) 0x63};
        // abc   GBK
        byte[] abc_GBK = {(byte) 0x61, (byte) 0x62, (byte) 0x63};
        // abc unicode
        byte[] abc_unicode = {(byte) 0x00, (byte) 0x61, (byte) 0x00, (byte) 0x62, (byte) 0x00, (byte) 0x63};
        test1(abc_ASCII, "ASCII");
        test1(abc_ISO88591, "ISO-8859-1");
        test1(abc_UTF8, "UTF-8");
        test1(abc_UTF16BE, "UTF-16BE");
        test1(abc_UTF16LE, "UTF-16LE");
        test1(abc_UTF16, "UTF-16");
        test1(abc_GBK, "GBK");
        test1(abc_unicode, "unicode");
        System.out.println(test1(abc_ASCII, "ASCII") + test1(abc_ISO88591, "ISO-8859-1")
                + test1(abc_UTF8, "UTF-8") + test1(abc_UTF16BE, "UTF-16BE") + test1(abc_UTF16LE, "UTF-16LE")
                + test1(abc_UTF16, "UTF-16") + test1(abc_GBK, "GBK") + test1(abc_unicode, "unicode"));
        // SIMPLE CHINESE
        byte[] zg_unicode = {(byte) 0x4E, (byte) 0x2D, (byte) 0x56, (byte) 0xFD};
        byte[] zg_UTF8 = {(byte) 0xE4, (byte) 0xB8, (byte) 0xAD, (byte) 0xE5, (byte) 0x9B, (byte) 0xBD};
        byte[] zg_UTF16BE = {(byte) 0x4E, (byte) 0x2D, (byte) 0x56, (byte) 0xFD};
        byte[] zg_UTF16LE = {(byte) 0x2D, (byte) 0x4E, (byte) 0xFD, (byte) 0x56};
        byte[] zg_UTF16 = {(byte) 0x4E, (byte) 0x2D, (byte) 0x56, (byte) 0xFD};
        byte[] zg_GBK = {(byte) 0xD6, (byte) 0xD0, (byte) 0xB9, (byte) 0xFA};
        test1(zg_UTF8, "UTF-8");
        test1(zg_UTF16BE, "UTF-16BE");
        test1(zg_UTF16LE, "UTF-16LE");
        test1(zg_UTF16, "UTF-16");
        test1(zg_GBK, "GBK");
        test1(zg_unicode, "unicode");
        System.out.println(test1(zg_UTF8, "UTF-8") + test1(zg_UTF16BE, "UTF-16BE")
                + test1(zg_UTF16LE, "UTF-16LE") + test1(zg_UTF16, "UTF-16") + test1(zg_GBK, "GBK")
                + test1(zg_unicode, "unicode"));
    }
    private static String test1(byte[] by, String str) {
        String result = new String(by, Charset.forName(str));
        System.out.println(result);
        return result;
    }
}
