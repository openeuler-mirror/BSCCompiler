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


import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;
public class TestStringEqualsCompress {
    static String code = "UTF-8";
    public static String compress(String str) throws IOException {
        if (str == null || str.length() == 0) {
            return str;
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        GZIPOutputStream gzip = new GZIPOutputStream(out);
        gzip.write(str.getBytes());
        gzip.close();
        return out.toString("ISO-8859-1");
    }
    public static String uncompress(String str) throws IOException {
        if (str == null || str.length() == 0) {
            return str;
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        ByteArrayInputStream in = new ByteArrayInputStream(str.getBytes("ISO-8859-1"));
        GZIPInputStream gunzip = new GZIPInputStream(in);
        byte[] buffer = new byte[256];
        int n;
        while ((n = gunzip.read(buffer)) >= 0) {
            out.write(buffer, 0, n);
        }
        return out.toString();
    }
    public static void main(String[] args) throws IOException {
        String longStrings = "Ahead-of-time compilation is possible as the compiler may just"
                + "convert an instruction thus: dex code: add-int v1000, v2000, v3000 C code: setIntRegter"
                + "(1000, call_dex_add_int(getIntRegister(2000), getIntRegister(3000)) This means even lid"
                + "instructions may have code generated, however, it is not expected that code generate in"
                + "this way will perform well. The job of AOT verification is to tell the compiler that"
                + "instructions are sound and provide tests to detect unsound sequences so slow path code"
                + "may be generated. Other than for totally invalid code, the verification may fail at AOr"
                + "run-time. At AOT time it can be because of incomplete information, at run-time it can e"
                + "that code in a different apk that the application depends upon has changed. The Dalvik"
                + "verifier would return a bool to state whether a Class were good or bad. In ART the fail"
                + "case becomes either a soft or hard failure. Classes have new states to represent that a"
                + "soft failure occurred at compile time and should be re-verified at run-time.";
        String zip = TestStringEqualsCompress.compress(longStrings);
        String unzip = TestStringEqualsCompress.uncompress(TestStringEqualsCompress.compress(longStrings));
        System.out.println(unzip.equals(longStrings));
        System.out.println(zip.equals(longStrings));
        System.out.println(unzip.equals(zip));
    }
}
