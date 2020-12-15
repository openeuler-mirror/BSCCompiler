/*
 *- @TestCaseID:maple/runtime/rc/optimization/RC_Array_14.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Scenario test for RC optimization: Test double array multi-threaded various scenarios, including:
 *                   1.Five copy scenarios: loop assignment, System.arraycopy(), Arrays.copyOf(), Arrays.copyOfRange(), clone()
 *                   2.Parameter modification / parameter has not been modified
 *                   3.final、static
 *                   4.As a constructor fun
 *                   5.Function call
 *                   6.Object Passing param
 *                   7.return constant; variable; function call
 *                   8.Exception
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: RC_Array_14.java
 *- @ExecuteClass: RC_Array_14
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.lang.reflect.Array;
import java.util.Arrays;
import java.io.PrintStream;

public class RC_Array_14 {
    static int check_count = 0;
    Thread sb = new Thread();
    double[] double_base1;
    double[][] double_base2;
    double[][][] double_base3;
    static double[] doubler1 = {1.1f,2.2f,3.3f,4.4f};
    static double[][] doubler2 = {{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}};
    static double[][][] doubler3 = {{{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}},{{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.3f}}};

    private static synchronized void incCheckCount() {
      check_count++;
    }

    /*The calling function returns a constant value as the assignment of the constant of this function.*/
    private double[] RC_Array_final10()
    {
        final double [] VALUE7 = {1.1f,2.2f,3.3f,4.8f};
        return VALUE7;
    }
    private double[][] RC_Array_final11()
    {
        final double[][] VALUE8 = {{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{2.7f}};
        return VALUE8;
    }
    private double[][][] RC_Array_final12()
    {
        final double[][][] VALUE9 = {{{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}},{{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.9f}}};
        return VALUE9;
    }

    private static void check(String test_method)
    {
        /*Public function: Check if the length of the assignment source array is as expected to determine whether the resource is recycled.*/
        if(doubler1.length == 4 && doubler2.length == 3 && doubler3.length == 2)
            incCheckCount();
        else
            System.out.println("ErrorResult in check: " + test_method);
    }

    public String  run(String argv[],PrintStream out) throws InterruptedException {
        String result = "Error"; /*STATUS_FAILED*/
        //Initialization check
        check("initialization");
        //Scene test
        Thread t1 = new Thread(new test01());
        Thread t2 = new Thread(new test02());
        Thread t3 = new Thread(new test03());
        Thread t4 = new Thread(new test04());
        Thread t5 = new Thread(new test05());
        Thread t6 = new Thread(new test06());
        Thread t7 = new Thread(new test07());
        Thread t8 = new Thread(new test08());
        Thread t9 = new Thread(new test09());
        //Copy scene multithreading
        t1.start();t2.start();t3.start();t4.start();t5.start();
        t1.join();t2.join();t3.join();t4.join();t5.join();
        //Other scene
        t6.start();t7.start();t8.start();
        t6.join();t7.join();t8.join();
        //Exception scene
        t9.start();
        t9.join();

        check("End");
        //Result judgment
        //System.out.println(check_count);
        if(check_count == 14)
            result = "ExpectResult";
        return result;
    }

    public static void main(String argv[]) {
        try {
            System.out.println(new RC_Array_14().run(argv, System.out));
        }catch (Exception e)
        {
            System.out.println(e);
        }
    }

    private class test01 implements Runnable{
        /*test01:One, two, three-dimensional array type cyclic assignment*/
        private void method01() {
            double [] tmp_double1 = new double[4];
            double [][] tmp_double2 = new double [3][4];
            double [][][] tmp_double3 = new double [2][3][4];
            //1D array
            for (int i = 0; i < doubler1.length; i++) {
                tmp_double1[i] = doubler1[i];
            }
            //2D array
            for (int i = 0; i < doubler2.length; i++)
                for(int j = 0; j < doubler2[i].length; j++)
                {
                    tmp_double2[i][j] = doubler2[i][j];
                }
            //3D array
            for (int i = 0; i < doubler3.length; i++)
                for(int j = 0; j < doubler3[i].length; j++)
                    for(int k = 0; k < doubler3[i][j].length; k++)
                    {
                        tmp_double3[i][j][k] = doubler3[i][j][k];
                    }
            //Compare the last value of the array correctly
            if(tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
                    incCheckCount();
            else
                System.out.println("ErrorResult in test01");
        }
        public void run()
        {
            method01();
        }
    }

    private class test02 implements Runnable{
        /*test02:One, two, three-dimensional array type System.arraycopy () assignment*/
        private void method02() {
            double [] tmp_double1 = new double[4];
            double [][] tmp_double2 = new double [3][4];
            double [][][] tmp_double3 = new double [2][3][4];
            System.arraycopy(doubler1,0,tmp_double1,0,doubler1.length);
            System.arraycopy(doubler2,0,tmp_double2,0,doubler2.length);
            System.arraycopy(doubler3,0,tmp_double3,0,doubler3.length);
            //Compare the last value of the array correctly
            if(tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test02");
        }
        public void run()
        {
            method02();
        }
    }

    private class test03 implements Runnable{
        /*test03:One, two, three-dimensional array type Arrays.copyOf() assignment*/
        private void method03()
        {
            double [] tmp_double1 = Arrays.copyOf(doubler1,doubler1.length);
            double [][] tmp_double2 = Arrays.copyOf(doubler2,doubler2.length);
            double [][][] tmp_double3 = Arrays.copyOf(doubler3,doubler3.length);
            //Compare the last value of the array correctly
            if(tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test03");
        }
        public void run()
        {
            method03();
        }
    }

    private class test04 implements Runnable{
        /*test04:One, two, three-dimensional array type Arrays.copyOfRange() assignment*/
        private void method04()
        {
            double [] tmp_double1 = Arrays.copyOfRange(doubler1,0,doubler1.length);
            double [][] tmp_double2 = Arrays.copyOfRange(doubler2,0,doubler2.length);
            double [][][] tmp_double3 = Arrays.copyOfRange(doubler3,0,doubler3.length);
            //Compare the last value of the array correctly
            if(tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test04");
        }
        public void run()
        {
            method04();
        }
    }

    private class test05 implements Runnable{
        /*test05:One, two, three-dimensional array type clone() assignment*/
        private void method05()
        {
            double [] tmp_double1 = doubler1.clone();
            double [][] tmp_double2 = doubler2.clone();
            double [][][] tmp_double3 = doubler3.clone();
            //Compare the last value of the array correctly
            if(tmp_double1[3] == (double) 4.4f && tmp_double2[2][0] == (double)7.7f && tmp_double3[1][2][0] == (double)10.3f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test05");
        }
        public void run()
        {
            method05();
        }
    }

    private class test06 implements Runnable{
        /*test06Interface call, internal initialization array, do not modify parameter values, only judge*/
        private void method06(double[] arr_dou1, double[][] arr_dou2, double[][][] arr_dou3)
        {
            double[] tmp_double1 = {1.1f,2.2f,3.3f,4.8f};
            double[][] tmp_double2 = {{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{2.7f}};
            double[][][] tmp_double3 = {{{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}},{{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.9f}}};
            //Check the values defined in the function
            if(tmp_double1[3] == 4.8f && tmp_double2[2][0] == 2.7f && tmp_double3[1][2][0] == 10.9f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test06 step1");
            //Check unmodified parameter values
            if(arr_dou1[3] == 4.4f && arr_dou2[2][0] == 7.7f && arr_dou3[1][2][0] == 10.3f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test06 step2");
        }
        public void run()
        {
            method06(doubler1, doubler2, doubler3);
        }
    }

    private class test07 implements Runnable{
        /*test07 Interface call, call function change to modify the parameter value and judge*/
        private Object change(Object temp1, Object temp2){
            temp1 = temp2;
            return temp1;
        }

        private void method07(double[] arr_dou1, double[][] arr_dou2, double[][][] arr_dou3) {
            double[] doubler1 = {1.1f,2.2f,3.3f,4.8f};
            double[][] doubler2 = {{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{2.7f}};
            double[][][] doubler3 = {{{1.1f,2.2f,3.3f,4.4f},{5.5f,6.6f},{7.7f}},{{4.1f,5.1f,6.1f,7.1f},{8.1f,9.1f},{10.9f}}};
            arr_dou1 = (double[]) change(arr_dou1,doubler1);
            arr_dou2 = (double[][])change(arr_dou2,doubler2);
            arr_dou3 = (double[][][]) change(arr_dou3,doubler3);
            //Check the values defined in the function
            if(doubler1[3] == 4.8f && doubler2[2][0] == 2.7f && doubler3[1][2][0] == 10.9f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test07 step1");
            //Check the modified parameter values
            if(arr_dou1[3] == 4.8f && arr_dou2[2][0] == 2.7f && arr_dou3[1][2][0] == 10.9f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test07 step2");
        }
        public void run()
        {
            method07(doubler1,doubler2,doubler3);
        }
    }

    private class test08 implements Runnable{
        /*Call the no-argument constructor, initialize the variables of the parent class, and assign values to the
        fields of the newly created object, and judge the result*/
        private void method08() {
            final double [] VALUE10 = RC_Array_final10();
            final double[][] VALUE11 = RC_Array_final11();
            final double[][][] VALUE12 = RC_Array_final12();
            RC_Array_14 rctest = new RC_Array_14();
            rctest.double_base1 = VALUE10;
            rctest.double_base2 = VALUE11;
            rctest.double_base3 = VALUE12;
            //Check the values defined in the function
            if(VALUE10[3] == 4.8f && VALUE11[2][0] == 2.7f && VALUE12[1][2][0] == 10.9f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test08 step1");
            //Check the modified parameter values
            if(rctest.double_base1[3] == 4.8f && rctest.double_base2[2][0] == 2.7f && rctest.double_base3[1][2][0] == 10.9f)
                incCheckCount();
            else
                System.out.println("ErrorResult in test08 step2");

        }
        public void run()
        {
            method08();
        }
    }

    private  class test09 implements Runnable{
        /*Exception test*/
        private synchronized void method09(){
            int check = 0;
            double [] value1 = RC_Array_final10();
            double[][] value2 = RC_Array_final11();
            double[][][] value3 = RC_Array_final12();
            //Is the value judged after the assignment?
            if(value1.length == 4 && value2.length == 3  && value3.length == 2)
                check++;
            else
                System.out.println("ErrorResult in test09——2");

            //ArrayIndexOutOfBoundsException
            try {
                value1[5] = 4.8f;
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            try {
                value2[5][0] = 4.8f;
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            try {
                value3[3][3][0] = 4.8f;
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            //IllegalArgumentException
            try {
                Array.getDouble(value2,1);
            }catch (IllegalArgumentException e){
                check++;
            }
            try {
                Array.getDouble(value3,1);
            }catch (IllegalArgumentException e){
                check++;
            }
            //Whether the judgment value is normal after the abnormality
            if(value1.length == 4 && value2.length == 3  && value3.length == 2)
                check++;
            else
                System.out.println("ErrorResult in test09——2");

            //NullPointerException
            value1 = null;
            value2 = null;
            value3 = null;
            try {
                Array.getDouble(value1, 1);
            } catch (NullPointerException e) {
                check++;
            }
            try {
                Array.getDouble(value2, 1);
            } catch (NullPointerException e) {
                check++;
            }
            try {
                Array.getDouble(value3, 1);
            } catch (NullPointerException e) {
                check++;
            }

            //System.out.println(check);
            if (check == 10)
                incCheckCount();
            else
                System.out.println("End: ErrorResult in test09");
        }
        public void run()
        {
            method09();
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
