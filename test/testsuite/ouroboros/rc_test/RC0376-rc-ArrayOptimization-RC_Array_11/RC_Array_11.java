/*
 *- @TestCaseID:maple/runtime/rc/optimization/RC_Array_11.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination: Scenario test for RC optimization: Test String array multi-threaded various scenarios, including:
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
 *- @Source: RC_Array_11.java
 *- @ExecuteClass: RC_Array_11
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.lang.reflect.Array;
import java.util.Arrays;
import java.io.PrintStream;

public class RC_Array_11 {
    static int check_count = 0;
    Thread sb = new Thread();
    String[] str_base1;
    String[][] str_base2;
    String[][][] str_base3;
    static String[] strr1 = {"'h''e''l''l''o'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"};
    static String[][] strr2 = {{"'h''e''l''l''o'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"},{"'2''0''1''8'","'1''1'"},{"'2''6'"}};
    static String[][][] strr3 = {{{"'h''e''l''l''o'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"},{"'2''0''1''8'","'1''1'"},{"'2''6'"}},
            {{"'h''u''a''w''e''i'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"},{"'2''0''1''8'","'1''1'"},{"'2''6'"}}};

    private static synchronized void intCheckCount() {
        check_count++;
    }

    /*The calling function returns a constant value as the assignment of the constant of this function.*/
    private String[] RC_Array_final01()
    {
        final String [] VALUE1 = {"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"};
        return VALUE1;
    }
    private String[][] RC_Array_final02()
    {
        final String[][] VALUE2 = {{"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"}, {"'2''0''1''8'","'1''1'"},{"'2''7'"}};
        return VALUE2;
    }
    private String[][][] RC_Array_final03()
    {
        final String[][][] VALUE3 = {{{"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"}, {"'2''0''1''8'","'1''1'"}, {"'2''7'"}},
                {{"'h''u''a''w''e''i'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"},{"'2''0''1''8'","'1''1'"},{"'2''7'"}}};
        return VALUE3;
    }

    private static void check(String test_method)
    {
        /*Public function: Check if the length of the assignment source array is as expected to determine whether the resource is recycled.*/
        if(strr1.length == 4 && strr2.length == 3 && strr3.length == 2)
            intCheckCount();
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
            System.out.println(new RC_Array_11().run(argv, System.out));
        }catch (Exception e)
        {
            System.out.println(e);
        }
    }

    private class test01 implements Runnable{
        /*test01:One, two, three-dimensional array type cyclic assignment*/
        private void method01() {
            String [] tmp_String1 = new String[4];
            String [][] tmp_String2 = new String [3][4];
            String [][][] tmp_String3 = new String [2][3][4];
            //1D array
            for (int i = 0; i < strr1.length; i++) {
                tmp_String1[i] = strr1[i];
            }
            //2D array
            for (int i = 0; i < strr2.length; i++)
                for(int j = 0; j < strr2[i].length; j++)
                {
                    tmp_String2[i][j] = strr2[i][j];
                }
            //3D array
            for (int i = 0; i < strr3.length; i++)
                for(int j = 0; j < strr3[i].length; j++)
                    for(int k = 0; k < strr3[i][j].length; k++)
                    {
                        tmp_String3[i][j][k] = strr3[i][j][k];
                    }
            //Compare the last value of the array correctly
            if(tmp_String1[3] == "'a''r''r''a''y'" && tmp_String2[2][0] == "'2''6'" && tmp_String3[1][2][0]== "'2''6'")
                    intCheckCount();
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
            String [] tmp_String1 = new String[4];
            String [][] tmp_String2 = new String [3][4];
            String [][][] tmp_String3 = new String [2][3][4];
            System.arraycopy(strr1,0,tmp_String1,0,strr1.length);
            System.arraycopy(strr2,0,tmp_String2,0,strr2.length);
            System.arraycopy(strr3,0,tmp_String3,0,strr3.length);
            //Compare the last value of the array correctly
            if(tmp_String1[3] == "'a''r''r''a''y'" && tmp_String2[2][0] == "'2''6'" && tmp_String3[1][2][0]== "'2''6'")
                intCheckCount();
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
            String [] tmp_String1 = Arrays.copyOf(strr1,strr1.length);
            String [][] tmp_String2 = Arrays.copyOf(strr2,strr2.length);
            String [][][] tmp_String3 = Arrays.copyOf(strr3,strr3.length);
            //Compare the last value of the array correctly
            if(tmp_String1[3] == "'a''r''r''a''y'" && tmp_String2[2][0] == "'2''6'" && tmp_String3[1][2][0]== "'2''6'")
                intCheckCount();
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
            String [] tmp_String1 = Arrays.copyOfRange(strr1,0,strr1.length);
            String [][] tmp_String2 = Arrays.copyOfRange(strr2,0,strr2.length);
            String [][][] tmp_String3 = Arrays.copyOfRange(strr3,0,strr3.length);
            //Compare the last value of the array correctly
            if(tmp_String1[3] == "'a''r''r''a''y'" && tmp_String2[2][0] == "'2''6'" && tmp_String3[1][2][0]== "'2''6'")
                intCheckCount();
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
            String [] tmp_String1 = strr1.clone();
            String [][] tmp_String2 = strr2.clone();
            String [][][] tmp_String3 = strr3.clone();
            //Compare the last value of the array correctly
            if(tmp_String1[3] == "'a''r''r''a''y'" && tmp_String2[2][0] == "'2''6'" && tmp_String3[1][2][0]== "'2''6'")
                intCheckCount();
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
        private void method06(String[] arr_str1, String[][] arr_str2, String[][][] arr_str3)
        {
            String[] tmp_String1 = {"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"};
            String[][] tmp_String2 = {{"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"}, {"'2''0''1''8'","'1''1'"},{"'2''7'"}};
            String[][][] tmp_String3 = {{{"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"}, {"'2''0''1''8'","'1''1'"}, {"'2''7'"}},
                    {{"'h''u''a''w''e''i'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"},{"'2''0''1''8'","'1''1'"},{"'2''7'"}}};
            //Check the values defined in the function
            if(tmp_String1[3] == "'h''a''l''l''y'" && tmp_String2[2][0] == "'2''7'" && tmp_String3[1][2][0] == "'2''7'")
                intCheckCount();
            else
                System.out.println("ErrorResult in test06 step1");
            //Check unmodified parameter values
            if(arr_str1[3]== "'a''r''r''a''y'" && arr_str2[2][0] == "'2''6'" && arr_str3[1][2][0] == "'2''6'")
                intCheckCount();
            else
                System.out.println("ErrorResult in test06 step2");
        }
        public void run()
        {
            method06(strr1, strr2, strr3);
        }
    }

    private class test07 implements Runnable{
        /*test07 Interface call, call function change to modify the parameter value and judge*/
        private Object change(Object temp1, Object temp2){
            temp1 = temp2;
            return temp1;
        }

        private void method07(String[] arr_str1, String[][] arr_str2, String[][][] arr_str3) {
            String[] strr1 = {"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"};
            String[][] strr2 = {{"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"}, {"'2''0''1''8'","'1''1'"},{"'2''7'"}};
            String[][][] strr3 = {{{"'h''h''l''l''o'","'t''e''s''t'","'f''o''r'","'h''a''l''l''y'"}, {"'2''0''1''8'","'1''1'"}, {"'2''7'"}},
                    {{"'h''u''a''w''e''i'","'t''e''s''t'","'f''o''r'","'a''r''r''a''y'"},{"'2''0''1''8'","'1''1'"},{"'2''7'"}}};
            arr_str1 = (String[]) change(arr_str1,strr1);
            arr_str2 = (String[][])change(arr_str2,strr2);
            arr_str3 = (String[][][]) change(arr_str3,strr3);
            //Check the values defined in the function
            if(strr1[3] == "'h''a''l''l''y'" && strr2[2][0] == "'2''7'" && strr3[1][2][0] == "'2''7'")
                intCheckCount();
            else
                System.out.println("ErrorResult in test07 step1");
            //Check the modified parameter values
            if(arr_str1[3]== "'h''a''l''l''y'" && arr_str2[2][0] == "'2''7'" && arr_str3[1][2][0] == "'2''7'")
                intCheckCount();
            else
                System.out.println("ErrorResult in test07 step2");
        }
        public void run()
        {
            method07(strr1,strr2,strr3);
        }
    }

    private class test08 implements Runnable{
        /*Call the no-argument constructor, initialize the variables of the parent class, and assign values to the
        fields of the newly created object, and judge the result*/
        private void method08() {
            final String [] VALUE1 = RC_Array_final01();
            final String[][] VALUE2 = RC_Array_final02();
            final String[][][] VALUE3 = RC_Array_final03();
            RC_Array_11 rctest = new RC_Array_11();
            rctest.str_base1 = VALUE1;
            rctest.str_base2 = VALUE2;
            rctest.str_base3 = VALUE3;
            //Check the values defined in the function
            if(VALUE1[3] == "'h''a''l''l''y'" && VALUE2[2][0] == "'2''7'" && VALUE3[1][2][0] == "'2''7'")
                intCheckCount();
            else
                System.out.println("ErrorResult in test08 step1");
            //Check the modified parameter values
            if(rctest.str_base1[3]== "'h''a''l''l''y'" && rctest.str_base2[2][0] == "'2''7'" && rctest.str_base3[1][2][0] == "'2''7'")
                intCheckCount();
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
            String [] value1 = RC_Array_final01();
            String[][] value2 = RC_Array_final02();
            String[][][] value3 = RC_Array_final03();
            //Is the value judged after the assignment?
            if(value1.length == 4 && value2.length == 3  && value3.length == 2)
                check++;
            else
                System.out.println("ErrorResult in test09——2");

            //ArrayIndexOutOfBoundsException
            try {
                value1[5] = "error";
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            try {
                value2[5][0] = "error";
            }catch (ArrayIndexOutOfBoundsException e){
                check++;
            }
            try {
                value3[3][3][0] = "error";
            }catch (ArrayIndexOutOfBoundsException e){
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
                Array.get(value1, 1);
            } catch (NullPointerException e) {
                check++;
            }
            try {
                Array.get(value2, 1);
            } catch (NullPointerException e) {
                check++;
            }
            try {
                Array.get(value3, 1);
            } catch (NullPointerException e) {
                check++;
            }

            //System.out.println(check);
            if (check == 8)
                intCheckCount();
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
