/*
 *- @TestCaseID:maple/runtime/rc/optimization/RC_Array_13.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Scenario test for RC optimization: Test boolean array multi-threaded various scenarios, including:
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
 *- @Source: RC_Array_13.java
 *- @ExecuteClass: RC_Array_13
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.lang.reflect.Array;
import java.util.Arrays;
import java.io.PrintStream;

public class RC_Array_13 {
    static int check_count = 0;
    Thread sb = new Thread();
    boolean[] bool_base1;
    boolean[][] bool_base2;
    boolean[][][] bool_base3;
    static boolean[] boolr1 = {true,false,false,true};
    static boolean[][] boolr2 = {{true,false,false,true},{false,false},{true}};
    static boolean[][][] boolr3 = {{{true,false,false,true},{false,false},{true}},{{false,true,false,true},{true,false},{false}}};

    private static synchronized void incCheckCount() {
      check_count++;
    }

    /*The calling function returns a constant value as the assignment of the constant of this function.*/
    private boolean[] RC_Array_final07()
    {
        final boolean [] VALUE7 = {true,false,false,false};
        return VALUE7;
    }
    private boolean[][] RC_Array_final08()
    {
        final boolean[][] VALUE8 = {{true,false,false,true},{false,false},{false}};
        return VALUE8;
    }
    private boolean[][][] RC_Array_final09()
    {
        final boolean[][][] VALUE9 = {{{true,false,false,true},{false,false},{true}},{{false,true,false,true},{true,false},{true}}};
        return VALUE9;
    }

    private static void check(String test_method)
    {
        /*Public function: Check if the length of the assignment source array is as expected to determine whether the resource is recycled.*/
        if(boolr1.length == 4 && boolr2.length == 3 && boolr3.length == 2)
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
        if(check_count == 14)
            result = "ExpectResult";
        return result;
    }

    public static void main(String argv[]) {
        try {
            System.out.println(new RC_Array_13().run(argv, System.out));
        }catch (Exception e)
        {
            System.out.println(e);
        }
    }

    private class test01 implements Runnable{
        /*test01:One, two, three-dimensional array type cyclic assignment*/
        private void method01() {
            boolean [] tmp_boolean1 = new boolean[4];
            boolean [][] tmp_boolean2 = new boolean [3][4];
            boolean [][][] tmp_boolean3 = new boolean [2][3][4];
            //1D array
            for (int i = 0; i < boolr1.length; i++) {
                tmp_boolean1[i] = boolr1[i];
            }
            //2D array
            for (int i = 0; i < boolr2.length; i++)
                for(int j = 0; j < boolr2[i].length; j++)
                {
                    tmp_boolean2[i][j] = boolr2[i][j];
                }
            //3D array
            for (int i = 0; i < boolr3.length; i++)
                for(int j = 0; j < boolr3[i].length; j++)
                    for(int k = 0; k < boolr3[i][j].length; k++)
                    {
                        tmp_boolean3[i][j][k] = boolr3[i][j][k];
                    }
            //Compare the last value of the array correctly
            if(tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false) {
                    incCheckCount();
            }
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
            boolean [] tmp_boolean1 = new boolean[4];
            boolean [][] tmp_boolean2 = new boolean [3][4];
            boolean [][][] tmp_boolean3 = new boolean [2][3][4];
            System.arraycopy(boolr1,0,tmp_boolean1,0,boolr1.length);
            System.arraycopy(boolr2,0,tmp_boolean2,0,boolr2.length);
            System.arraycopy(boolr3,0,tmp_boolean3,0,boolr3.length);
            //Compare the last value of the array correctly
            if(tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false)
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
            boolean [] tmp_boolean1 = Arrays.copyOf(boolr1,boolr1.length);
            boolean [][] tmp_boolean2 = Arrays.copyOf(boolr2,boolr2.length);
            boolean [][][] tmp_boolean3 = Arrays.copyOf(boolr3,boolr3.length);
            //Compare the last value of the array correctly
            if(tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false)
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
            boolean [] tmp_boolean1 = Arrays.copyOfRange(boolr1,0,boolr1.length);
            boolean [][] tmp_boolean2 = Arrays.copyOfRange(boolr2,0,boolr2.length);
            boolean [][][] tmp_boolean3 = Arrays.copyOfRange(boolr3,0,boolr3.length);
            //Compare the last value of the array correctly
            if(tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false)
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
            boolean [] tmp_boolean1 = boolr1.clone();
            boolean [][] tmp_boolean2 = boolr2.clone();
            boolean [][][] tmp_boolean3 = boolr3.clone();
            //Compare the last value of the array correctly
            if(tmp_boolean1[3] == true && tmp_boolean2[2][0] == true && tmp_boolean3[1][2][0] == false)
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
        private void method06(boolean[] arr_bool1, boolean[][] arr_bool2, boolean[][][] arr_bool3)
        {
            boolean[] tmp_boolean1 = {true,false,false,false};
            boolean[][] tmp_boolean2 = {{true,false,false,true},{false,false},{false}};
            boolean[][][] tmp_boolean3 = {{{true,false,false,true},{false,false},{true}},{{false,true,false,true},{true,false},{true}}};
            //Check the values defined in the function
            if(tmp_boolean1[3] == false && tmp_boolean2[2][0] == false && tmp_boolean3[1][2][0] == true)
                incCheckCount();
            else
                System.out.println("ErrorResult in test06 step1");
            //Check unmodified parameter values
            if(arr_bool1[3] == true && arr_bool2[2][0] == true && arr_bool3[1][2][0] == false)
                incCheckCount();
            else
                System.out.println("ErrorResult in test06 step2");
        }
        public void run()
        {
            method06(boolr1, boolr2, boolr3);
        }
    }

    private class test07 implements Runnable{
        /*test07 Interface call, call function change to modify the parameter value and judge*/
        private Object change(Object temp1, Object temp2){
            temp1 = temp2;
            return temp1;
        }

        private void method07(boolean[] arr_bool1, boolean[][] arr_bool2,boolean[][][] arr_bool3) {
            boolean[] boolr1 = {true,false,false,false};
            boolean[][] boolr2 = {{true,false,false,true},{false,false},{false}};
            boolean[][][] boolr3 = {{{true,false,false,true},{false,false},{true}},{{false,true,false,true},{true,false},{true}}};
            arr_bool1 = (boolean[]) change(arr_bool1,boolr1);
            arr_bool2 = (boolean[][])change(arr_bool2,boolr2);
            arr_bool3 = (boolean[][][]) change(arr_bool3,boolr3);
            //Check the values defined in the function
            if(boolr1[3] == false && boolr2[2][0] == false && boolr3[1][2][0] == true)
                incCheckCount();
            else
                System.out.println("ErrorResult in test07 step1");
            //Check the modified parameter values
            if(arr_bool1[3] == false && arr_bool2[2][0] == false && arr_bool3[1][2][0] == true)
                incCheckCount();
            else
                System.out.println("ErrorResult in test07 step2");
        }
        public void run()
        {
            method07(boolr1,boolr2,boolr3);
        }
    }

    private class test08 implements Runnable{
        /*Call the no-argument constructor, initialize the variables of the parent class, and assign values to the
        fields of the newly created object, and judge the result*/
        private void method08() {
            final boolean [] VALUE7 = RC_Array_final07();
            final boolean[][] VALUE8 = RC_Array_final08();
            final boolean[][][] VALUE9 = RC_Array_final09();
            RC_Array_13 rctest = new RC_Array_13();
            rctest.bool_base1 = VALUE7;
            rctest.bool_base2 = VALUE8;
            rctest.bool_base3 = VALUE9;
            //Check the values defined in the function
            if(VALUE7[3] == false && VALUE8[2][0] == false && VALUE9[1][2][0] == true)
                incCheckCount();
            else
                System.out.println("ErrorResult in test08 step1");
            //Check the modified parameter values
            if(rctest.bool_base1[3] == false && rctest.bool_base2[2][0] == false && rctest.bool_base3[1][2][0] == true)
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
            boolean [] value1 = RC_Array_final07();
            boolean[][] value2 = RC_Array_final08();
            boolean[][][] value3 = RC_Array_final09();
            //Is the value judged after the assignment?
            if(value1.length == 4 && value2.length == 3  && value3.length == 2)
                check++;
            else
                System.out.println("ErrorResult in test09——2");

            //ArrayIndexOutOfBoundsException
            try {
                value1[5] = true;
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            try {
                value2[5][0] = true;
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            try {
                value3[3][3][0] = true;
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            //IllegalArgumentException
            try {
                Array.getBoolean(value2,1);
            }catch (IllegalArgumentException e){
                check++;
            }
            try {
                Array.getBoolean(value3,1);
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
                Array.getBoolean(value1, 1);
            } catch (NullPointerException e) {
                check++;
            }
            try {
                Array.getBoolean(value2, 1);
            } catch (NullPointerException e) {
                check++;
            }
            try {
                Array.getBoolean(value3, 1);
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
