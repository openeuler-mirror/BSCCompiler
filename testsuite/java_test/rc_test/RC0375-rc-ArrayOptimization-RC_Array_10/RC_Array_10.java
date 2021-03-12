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


import java.util.Arrays;
public class RC_Array_10 {
    static int check_count = 0;
    //Nine definitions:int、string、char、boolean、byte、short、long、float、double
    static int[] intr1 = {10,20,30,40};
    static int[][] intr2 = {{10,20,30,40},{40,50},{60}};
    static int[][] intr21 = {{40,50,60,30},{70,80},{90}};
    static int[][][] intr3 = {{{10,20,30,40},{40,50},{60}},{{40,50,60,30},{70,80},{90}}};
    static String[] strr1 = {"hello","test","for","array"};
    static String[][] strr2 = {{"hello","test","for","array"},{"2018","11"},{"26"}};
    static String[][] strr21 = {{"huawei","test","for","array"},{"2018","11"},{"26"}};
    static String[][][] strr3 = {{{"hello","test","for","array"},{"2018","11"},{"26"}},{{"huawei","test","for","array"},{"2018","11"},{"26"}}};
    static char[] charr1 = {'a','b','c','d'};
    static char[][] charr2 = {{'a','b','c','d'},{'e','f'},{'g'}};
    static char[][] charr21 = {{'h','i','j','k'},{'l','m'},{'n'}};
    static char[][][] charr3 = {{{'a','b','c','d'},{'e','f'},{'g'}},{{'h','i','j','k'},{'l','m'},{'n'}}};
    static boolean[] boolr1 = {true,false,false,true};
    static boolean[][] boolr2 = {{true,false,false,true},{false,false},{true}};
    static boolean[][] boolr21 = {{false,true,false,true},{true,false},{false}};
    static boolean[][][] boolr3 = {{{true,false,false,true},{false,false},{true}},{{false,true,false,true},{true,false},{false}}};
    static byte[] byter1 = {1,2,3,4};
    static byte[][] byter2 = {{1,2,3,4},{5,6},{7}};
    static byte[][] byter21 = {{4,5,6,7},{8,9},{10}};
    static byte[][][] byter3 = {{{1,2,3,4},{5,6},{7}},{{4,5,6,7},{8,9},{10}}};
    static short[] shortr1 = {1,2,3,4};
    static short[][] shortr2 = {{1,2,3,4},{5,6},{7}};
    static short[][] shortr21 = {{4,5,6,7},{8,9},{10}};
    static short[][][] shortr3 = {{{1,2,3,4},{5,6},{7}},{{4,5,6,7},{8,9},{10}}};
    static long[] longr1 = {1,2,3,4};
    static long[][] longr2 = {{1,2,3,4},{5,6},{7}};
    static long[][] longr21 = {{4,5,6,7},{8,9},{10}};
    static long[][][] longr3 = {{{1,2,3,4},{5,6},{7}},{{4,5,6,7},{8,9},{10}}};
    static float[] floatr1 = {1.1f,2.2f,3.3f,4.4f};
    static float[][] floatr2 = {{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}};
    static float[][] floatr21 = {{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.3f}};
    static float[][][] floatr3 = {{{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}},{{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.3f}}};
    static double[] doubler1 = {1.1f,2.2f,3.3f,4.4f};
    static double[][] doubler2 = {{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}};
    static double[][] doubler21 = {{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.3f}};
    static double[][][] doubler3 = {{{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}},{{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.3f}}};
    public static void main(String [] args) {
        //Initialization check
        check("initialization");
        //Copy scene1：One, two, three-dimensional array type cyclic assignment
        test01();
        check("test01");
        //Copy scene2：One, two, three-dimensional array type System.arraycopy () assignment
        test02();
        check("test02");
        //Copy scene3：One, two, three-dimensional array type Arrays.copyOf() assignment
        test03();
        check("test03");
        //Copy scene4：One, two, three-dimensional array type Arrays.copyOfRange() assignment
        test04();
        check("test04");
        //Copy scene5：One, two, three-dimensional array type clone() assignment
        test05();
        check("test05");
        //Result judgment
        //System.out.println(check_count);
        if(check_count == 11)
            System.out.println("ExpectResult");
    }
    private static void check(String test_method)
    {
        /*Public function: Check if the length of the assignment source array is as expected to determine whether the resource is recycled*/
        if(intr1.length == 4 && intr2.length == 3 && intr21.length == 3 && intr3.length == 2
                && strr1.length == 4 && strr2.length == 3 && strr21.length == 3 && strr3.length == 2
                && charr1.length == 4 && charr2.length == 3 && charr21.length == 3 && charr3.length == 2
                && boolr1.length == 4 && boolr2.length == 3 && boolr21.length == 3 && boolr3.length == 2
                && byter1.length == 4 && byter2.length == 3 && byter21.length == 3 && byter3.length == 2
                && shortr1.length == 4 && shortr2.length == 3 && shortr21.length == 3 && shortr3.length == 2
                && longr1.length == 4 && longr2.length == 3 && longr21.length == 3 && longr3.length == 2
                && floatr1.length == 4 && floatr2.length == 3 && floatr21.length == 3 && floatr3.length == 2
                && doubler1.length == 4 && doubler2.length == 3 && doubler21.length == 3 && doubler3.length == 2)
            check_count++;
        else
            System.out.println("ErrorResult in check: " + test_method);
    }
    private static void test01() {
        /*test01:One, two, three-dimensional array type cyclic assignment*/
        int [] tmp_int1 = new int[4];
        int [][] tmp_int2 = new int [3][4];
        int [][][] tmp_int3 = new int [2][3][4];
        String [] tmp_String1 = new String[4];
        String [][] tmp_String2 = new String [3][4];
        String [][][] tmp_String3 = new String [2][3][4];
        char [] tmp_char1 = new char[4];
        char [][] tmp_char2 = new char [3][4];
        char [][][] tmp_char3 = new char [2][3][4];
        boolean [] tmp_boolean1 = new boolean[4];
        boolean [][] tmp_boolean2 = new boolean [3][4];
        boolean [][][] tmp_boolean3 = new boolean [2][3][4];
        byte [] tmp_byte1 = new byte[4];
        byte [][] tmp_byte2 = new byte [3][4];
        byte [][][] tmp_byte3 = new byte [2][3][4];
        short [] tmp_short1 = new short[4];
        short [][] tmp_short2 = new short [3][4];
        short [][][] tmp_short3 = new short [2][3][4];
        long [] tmp_long1 = new long[4];
        long [][] tmp_long2 = new long [3][4];
        long [][][] tmp_long3 = new long [2][3][4];
        float [] tmp_float1 = new float[4];
        float [][] tmp_float2 = new float [3][4];
        float [][][] tmp_float3 = new float [2][3][4];
        double [] tmp_double1 = new double[4];
        double [][] tmp_double2 = new double [3][4];
        double [][][] tmp_double3 = new double [2][3][4];
        //1D array
        for (int i = 0; i < intr1.length; i++) {
            tmp_int1[i] = intr1[i];
            tmp_String1[i] = strr1[i];
            tmp_char1[i] = charr1[i];
            tmp_boolean1[i] = boolr1[i];
            tmp_byte1[i] = byter1[i];
            tmp_short1[i] = shortr1[i];
            tmp_long1[i] = longr1[i];
            tmp_float1[i] = floatr1[i];
            tmp_double1[i] = doubler1[i];
        }
        //2D array
        for (int i = 0; i < intr2.length; i++)
            for(int j = 0; j < intr2[i].length; j++)
            {
                tmp_int2[i][j] = intr2[i][j];
                tmp_String2[i][j] = strr2[i][j];
                tmp_char2[i][j] = charr2[i][j];
                tmp_boolean2[i][j] = boolr2[i][j];
                tmp_byte2[i][j] = byter2[i][j];
                tmp_short2[i][j] = shortr2[i][j];
                tmp_long2[i][j] = longr2[i][j];
                tmp_float2[i][j] = floatr2[i][j];
                tmp_double2[i][j] = doubler2[i][j];
            }
        //3D array
        for (int i = 0; i < intr3.length; i++)
            for(int j = 0; j < intr3[i].length; j++)
                for(int k = 0; k < intr3[i][j].length; k++)
                {
                    tmp_int3[i][j][k] = intr3[i][j][k];
                    tmp_String3[i][j][k] = strr3[i][j][k];
                    tmp_char3[i][j][k] = charr3[i][j][k];
                    tmp_boolean3[i][j][k] = boolr3[i][j][k];
                    tmp_byte3[i][j][k] = byter3[i][j][k];
                    tmp_short3[i][j][k] = shortr3[i][j][k];
                    tmp_long3[i][j][k] = longr3[i][j][k];
                    tmp_float3[i][j][k] = floatr3[i][j][k];
                    tmp_double3[i][j][k] = doubler3[i][j][k];
                }
        //Compare the last value of the array correctly
        if(tmp_int1[3] == 40 && tmp_int2[2][0] == 60 && tmp_int3[1][2][0] == 90
                && tmp_String1[3] == "array" && tmp_String2[2][0] == "26" && tmp_String3[1][2][0]== "26"
                && tmp_char1[3] == 'd' && tmp_char2[2][0]== 'g' && tmp_char3[1][2][0] == 'n'
                && tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false
                && tmp_byte1[3] == (byte)4 && tmp_byte2[2][0] == (byte)7 && tmp_byte3[1][2][0] == (byte)10
                && tmp_short1[3] == (short)4 && tmp_short2[2][0] == (short)7 && tmp_short3[1][2][0] == (short)10
                && tmp_long1[3] == (long)4 && tmp_long2[2][0] == (long)7 && tmp_long3[1][2][0] == (long)10
                && tmp_float1[3] == 4.4f && tmp_float2[2][0] == 7.7f && tmp_float3[1][2][0] == 10.3f
                && tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
            check_count++;
        else
            System.out.println("ErrorResult in test01");
    }
    private static void test02() {
        /*test02:One, two, three-dimensional array type System.arraycopy () assignment*/
        int [] tmp_int1 = new int[4];
        int [][] tmp_int2 = new int [3][4];
        int [][][] tmp_int3 = new int [2][3][4];
        String [] tmp_String1 = new String[4];
        String [][] tmp_String2 = new String [3][4];
        String [][][] tmp_String3 = new String [2][3][4];
        char [] tmp_char1 = new char[4];
        char [][] tmp_char2 = new char [3][4];
        char [][][] tmp_char3 = new char [2][3][4];
        boolean [] tmp_boolean1 = new boolean[4];
        boolean [][] tmp_boolean2 = new boolean [3][4];
        boolean [][][] tmp_boolean3 = new boolean [2][3][4];
        byte [] tmp_byte1 = new byte[4];
        byte [][] tmp_byte2 = new byte [3][4];
        byte [][][] tmp_byte3 = new byte [2][3][4];
        short [] tmp_short1 = new short[4];
        short [][] tmp_short2 = new short [3][4];
        short [][][] tmp_short3 = new short [2][3][4];
        long [] tmp_long1 = new long[4];
        long [][] tmp_long2 = new long [3][4];
        long [][][] tmp_long3 = new long [2][3][4];
        float [] tmp_float1 = new float[4];
        float [][] tmp_float2 = new float [3][4];
        float [][][] tmp_float3 = new float [2][3][4];
        double [] tmp_double1 = new double[4];
        double [][] tmp_double2 = new double [3][4];
        double [][][] tmp_double3 = new double [2][3][4];
        System.arraycopy(intr1,0,tmp_int1,0,intr1.length);
        System.arraycopy(intr2,0,tmp_int2,0,intr2.length);
        System.arraycopy(intr3,0,tmp_int3,0,intr3.length);
        System.arraycopy(strr1,0,tmp_String1,0,strr1.length);
        System.arraycopy(strr2,0,tmp_String2,0,strr2.length);
        System.arraycopy(strr3,0,tmp_String3,0,strr3.length);
        System.arraycopy(charr1,0,tmp_char1,0,charr1.length);
        System.arraycopy(charr2,0,tmp_char2,0,charr2.length);
        System.arraycopy(charr3,0,tmp_char3,0,charr3.length);
        System.arraycopy(boolr1,0,tmp_boolean1,0,boolr1.length);
        System.arraycopy(boolr2,0,tmp_boolean2,0,boolr2.length);
        System.arraycopy(boolr3,0,tmp_boolean3,0,boolr3.length);
        System.arraycopy(byter1,0,tmp_byte1,0,byter1.length);
        System.arraycopy(byter2,0,tmp_byte2,0,byter2.length);
        System.arraycopy(byter3,0,tmp_byte3,0,byter3.length);
        System.arraycopy(shortr1,0,tmp_short1,0,shortr1.length);
        System.arraycopy(shortr2,0,tmp_short2,0,shortr2.length);
        System.arraycopy(shortr3,0,tmp_short3,0,shortr3.length);
        System.arraycopy(longr1,0,tmp_long1,0,longr1.length);
        System.arraycopy(longr2,0,tmp_long2,0,longr2.length);
        System.arraycopy(longr3,0,tmp_long3,0,longr3.length);
        System.arraycopy(floatr1,0,tmp_float1,0,floatr1.length);
        System.arraycopy(floatr2,0,tmp_float2,0,floatr2.length);
        System.arraycopy(floatr3,0,tmp_float3,0,floatr3.length);
        System.arraycopy(doubler1,0,tmp_double1,0,doubler1.length);
        System.arraycopy(doubler2,0,tmp_double2,0,doubler2.length);
        System.arraycopy(doubler3,0,tmp_double3,0,doubler3.length);
        //Compare the last value of the array correctly
        if(tmp_int1[3] == 40 && tmp_int2[2][0] == 60 && tmp_int3[1][2][0] == 90
                && tmp_String1[3] == "array" && tmp_String2[2][0] == "26" && tmp_String3[1][2][0]== "26"
                && tmp_char1[3] == 'd' && tmp_char2[2][0]== 'g' && tmp_char3[1][2][0] == 'n'
                && tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false
                && tmp_byte1[3] == (byte)4 && tmp_byte2[2][0] == (byte)7 && tmp_byte3[1][2][0] == (byte)10
                && tmp_short1[3] == (short)4 && tmp_short2[2][0] == (short)7 && tmp_short3[1][2][0] == (short)10
                && tmp_long1[3] == (long)4 && tmp_long2[2][0] == (long)7 && tmp_long3[1][2][0] == (long)10
                && tmp_float1[3] == 4.4f && tmp_float2[2][0] == 7.7f && tmp_float3[1][2][0] == 10.3f
                && tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
            check_count++;
        else
            System.out.println("ErrorResult in test02");
    }
    private static void test03()
    {
        /*test03:One, two, three-dimensional array type Arrays.copyOf() assignment*/
        int [] tmp_int1 = Arrays.copyOf(intr1,intr1.length);
        int [][] tmp_int2 = Arrays.copyOf(intr2,intr2.length);
        int [][][] tmp_int3 = Arrays.copyOf(intr3,intr3.length);
        String [] tmp_String1 = Arrays.copyOf(strr1,strr1.length);
        String [][] tmp_String2 = Arrays.copyOf(strr2,strr2.length);
        String [][][] tmp_String3 = Arrays.copyOf(strr3,strr3.length);
        char [] tmp_char1 = Arrays.copyOf(charr1,charr1.length);
        char [][] tmp_char2 = Arrays.copyOf(charr2,charr2.length);
        char [][][] tmp_char3 = Arrays.copyOf(charr3,charr3.length);
        boolean [] tmp_boolean1 = Arrays.copyOf(boolr1,boolr1.length);
        boolean [][] tmp_boolean2 = Arrays.copyOf(boolr2,boolr2.length);
        boolean [][][] tmp_boolean3 = Arrays.copyOf(boolr3,boolr3.length);
        byte [] tmp_byte1 = Arrays.copyOf(byter1,byter1.length);
        byte [][] tmp_byte2 = Arrays.copyOf(byter2,byter2.length);
        byte [][][] tmp_byte3 = Arrays.copyOf(byter3,byter3.length);
        short [] tmp_short1 = Arrays.copyOf(shortr1,shortr1.length);
        short [][] tmp_short2 = Arrays.copyOf(shortr2,shortr2.length);
        short [][][] tmp_short3 = Arrays.copyOf(shortr3,shortr3.length);
        long [] tmp_long1 = Arrays.copyOf(longr1,longr1.length);
        long [][] tmp_long2 = Arrays.copyOf(longr2,longr2.length);
        long [][][] tmp_long3 = Arrays.copyOf(longr3,longr3.length);
        float [] tmp_float1 = Arrays.copyOf(floatr1,floatr1.length);
        float [][] tmp_float2 = Arrays.copyOf(floatr2,floatr2.length);
        float [][][] tmp_float3 = Arrays.copyOf(floatr3,floatr3.length);
        double [] tmp_double1 = Arrays.copyOf(doubler1,doubler1.length);
        double [][] tmp_double2 = Arrays.copyOf(doubler2,doubler2.length);
        double [][][] tmp_double3 = Arrays.copyOf(doubler3,doubler3.length);
        //Compare the last value of the array correctly
        if(tmp_int1[3] == 40 && tmp_int2[2][0] == 60 && tmp_int3[1][2][0] == 90
                && tmp_String1[3] == "array" && tmp_String2[2][0] == "26" && tmp_String3[1][2][0]== "26"
                && tmp_char1[3] == 'd' && tmp_char2[2][0]== 'g' && tmp_char3[1][2][0] == 'n'
                && tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false
                && tmp_byte1[3] == (byte)4 && tmp_byte2[2][0] == (byte)7 && tmp_byte3[1][2][0] == (byte)10
                && tmp_short1[3] == (short)4 && tmp_short2[2][0] == (short)7 && tmp_short3[1][2][0] == (short)10
                && tmp_long1[3] == (long)4 && tmp_long2[2][0] == (long)7 && tmp_long3[1][2][0] == (long)10
                && tmp_float1[3] == 4.4f && tmp_float2[2][0] == 7.7f && tmp_float3[1][2][0] == 10.3f
                && tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
            check_count++;
        else
            System.out.println("ErrorResult in test03");
    }
    private static void test04()
    {
        /*test04:One, two, three-dimensional array type Arrays.copyOfRange() assignment*/
        int [] tmp_int1 = Arrays.copyOfRange(intr1,0,intr1.length);
        int [][] tmp_int2 = Arrays.copyOfRange(intr2,0,intr2.length);
        int [][][] tmp_int3 = Arrays.copyOfRange(intr3,0,intr3.length);
        String [] tmp_String1 = Arrays.copyOfRange(strr1,0,strr1.length);
        String [][] tmp_String2 = Arrays.copyOfRange(strr2,0,strr2.length);
        String [][][] tmp_String3 = Arrays.copyOfRange(strr3,0,strr3.length);
        char [] tmp_char1 = Arrays.copyOfRange(charr1,0,charr1.length);
        char [][] tmp_char2 = Arrays.copyOfRange(charr2,0,charr2.length);
        char [][][] tmp_char3 = Arrays.copyOfRange(charr3,0,charr3.length);
        boolean [] tmp_boolean1 = Arrays.copyOfRange(boolr1,0,boolr1.length);
        boolean [][] tmp_boolean2 = Arrays.copyOfRange(boolr2,0,boolr2.length);
        boolean [][][] tmp_boolean3 = Arrays.copyOfRange(boolr3,0,boolr3.length);
        byte [] tmp_byte1 = Arrays.copyOfRange(byter1,0,byter1.length);
        byte [][] tmp_byte2 = Arrays.copyOfRange(byter2,0,byter2.length);
        byte [][][] tmp_byte3 = Arrays.copyOfRange(byter3,0,byter3.length);
        short [] tmp_short1 = Arrays.copyOfRange(shortr1,0,shortr1.length);
        short [][] tmp_short2 = Arrays.copyOfRange(shortr2,0,shortr2.length);
        short [][][] tmp_short3 = Arrays.copyOfRange(shortr3,0,shortr3.length);
        long [] tmp_long1 = Arrays.copyOfRange(longr1,0,longr1.length);
        long [][] tmp_long2 = Arrays.copyOfRange(longr2,0,longr2.length);
        long [][][] tmp_long3 = Arrays.copyOfRange(longr3,0,longr3.length);
        float [] tmp_float1 = Arrays.copyOfRange(floatr1,0,floatr1.length);
        float [][] tmp_float2 = Arrays.copyOfRange(floatr2,0,floatr2.length);
        float [][][] tmp_float3 = Arrays.copyOfRange(floatr3,0,floatr3.length);
        double [] tmp_double1 = Arrays.copyOfRange(doubler1,0,doubler1.length);
        double [][] tmp_double2 = Arrays.copyOfRange(doubler2,0,doubler2.length);
        double [][][] tmp_double3 = Arrays.copyOfRange(doubler3,0,doubler3.length);
        //Compare the last value of the array correctly
        if(tmp_int1[3] == 40 && tmp_int2[2][0] == 60 && tmp_int3[1][2][0] == 90
                && tmp_String1[3] == "array" && tmp_String2[2][0] == "26" && tmp_String3[1][2][0]== "26"
                && tmp_char1[3] == 'd' && tmp_char2[2][0]== 'g' && tmp_char3[1][2][0] == 'n'
                && tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false
                && tmp_byte1[3] == (byte)4 && tmp_byte2[2][0] == (byte)7 && tmp_byte3[1][2][0] == (byte)10
                && tmp_short1[3] == (short)4 && tmp_short2[2][0] == (short)7 && tmp_short3[1][2][0] == (short)10
                && tmp_long1[3] == (long)4 && tmp_long2[2][0] == (long)7 && tmp_long3[1][2][0] == (long)10
                && tmp_float1[3] == 4.4f && tmp_float2[2][0] == 7.7f && tmp_float3[1][2][0] == 10.3f
                && tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
            check_count++;
        else
            System.out.println("ErrorResult in test04");
    }
    private static void test05()
    {
        /*test05:One, two, three-dimensional array type clone() assignment*/
        int [] tmp_int1 = intr1.clone();
        int [][] tmp_int2 = intr2.clone();
        int [][][] tmp_int3 = intr3.clone();
        String [] tmp_String1 = strr1.clone();
        String [][] tmp_String2 = strr2.clone();
        String [][][] tmp_String3 = strr3.clone();
        char [] tmp_char1 = charr1.clone();
        char [][] tmp_char2 = charr2.clone();
        char [][][] tmp_char3 = charr3.clone();
        boolean [] tmp_boolean1 = boolr1.clone();
        boolean [][] tmp_boolean2 = boolr2.clone();
        boolean [][][] tmp_boolean3 = boolr3.clone();
        byte [] tmp_byte1 = byter1.clone();
        byte [][] tmp_byte2 = byter2.clone();
        byte [][][] tmp_byte3 = byter3.clone();
        short [] tmp_short1 = shortr1.clone();
        short [][] tmp_short2 = shortr2.clone();
        short [][][] tmp_short3 = shortr3.clone();
        long [] tmp_long1 = longr1.clone();
        long [][] tmp_long2 = longr2.clone();
        long [][][] tmp_long3 = longr3.clone();
        float [] tmp_float1 = floatr1.clone();
        float [][] tmp_float2 = floatr2.clone();
        float [][][] tmp_float3 = floatr3.clone();
        double [] tmp_double1 = doubler1.clone();
        double [][] tmp_double2 = doubler2.clone();
        double [][][] tmp_double3 = doubler3.clone();
        //Compare the last value of the array correctly
        if(tmp_int1[3] == 40 && tmp_int2[2][0] == 60 && tmp_int3[1][2][0] == 90
                && tmp_String1[3] == "array" && tmp_String2[2][0] == "26" && tmp_String3[1][2][0]== "26"
                && tmp_char1[3] == 'd' && tmp_char2[2][0]== 'g' && tmp_char3[1][2][0] == 'n'
                && tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false
                && tmp_byte1[3] == (byte)4 && tmp_byte2[2][0] == (byte)7 && tmp_byte3[1][2][0] == (byte)10
                && tmp_short1[3] == (short)4 && tmp_short2[2][0] == (short)7 && tmp_short3[1][2][0] == (short)10
                && tmp_long1[3] == (long)4 && tmp_long2[2][0] == (long)7 && tmp_long3[1][2][0] == (long)10
                && tmp_float1[3] == 4.4f && tmp_float2[2][0] == 7.7f && tmp_float3[1][2][0] == 10.3f
                && tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
            check_count++;
        else
            System.out.println("ErrorResult in test05");
    }
}