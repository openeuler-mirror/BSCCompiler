/*
 *- @TestCaseID:maple/runtime/rc/optimization/RC_Array_12.java
 *- @TestCaseName:MyselfClassName
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title/Destination:Scenario test for RC optimization: Test Object array multi-threaded various scenarios, including:
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
 *- @Source: RC_Array_12.java
 *- @ExecuteClass: RC_Array_12
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.lang.reflect.Array;
import java.util.Arrays;
import java.io.PrintStream;

public class RC_Array_12 {
    static int check_count = 0;
    Thread sb = new Thread();
    Object[] obj_base1;
    Object[][] obj_base2;
    Object[][][] obj_base3;
    static Object[] objr1 = {'a','b','c','d'};
    static Object[][] objr2 = {{'a','b','c','d'},{'e','f'},{'g'}};
    static Object[][][] objr3 = {{{'a','b','c','d'},{'e','f'},{'g'}},{{'h','i','j','k'},{'l','m'},{'n'}}};

    private static synchronized void incCheckCount() {
      check_count++;
    }

    /*The calling function returns a constant value as the assignment of the constant of this function.*/
    private Object[] RC_Array_final04()
    {
        final Object [] VALUE4 = {'b','s','d','g'};
        return VALUE4;
    }
    private Object[][] RC_Array_final05()
    {
        final Object[][] VALUE5 = {{'b','s','d','g'},{'e','f'},{'s'}};
        return VALUE5;
    }
    private Object[][][] RC_Array_final06()
    {
        final Object[][][] VALUE6 = {{{'b','s','d','g'},{'e','f'},{'s'}},{{'h','i','j','k'},{'l','m'},{'a'}}};
        return VALUE6;
    }

    private static void check(String test_method)
    {
        /*Public function: Check if the length of the assignment source array is as expected to determine whether the resource is recycled.*/
        if(objr1.length == 4 && objr2.length == 3 && objr3.length == 2)
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
            System.out.println(new RC_Array_12().run(argv, System.out));
        }catch (Exception e)
        {
            System.out.println(e);
        }
    }

    private class test01 implements Runnable{
        /*test01:One, two, three-dimensional array type cyclic assignment*/
        private void method01() {
            Object [] tmp_obj1 = new Object[4];
            Object [][] tmp_obj2 = new Object [3][4];
            Object [][][] tmp_obj3 = new Object [2][3][4];
            //1D array
            for (int i = 0; i < objr1.length; i++) {
                tmp_obj1[i] = objr1[i];
            }
            //2D array
            for (int i = 0; i < objr2.length; i++)
                for(int j = 0; j < objr2[i].length; j++)
                {
                    tmp_obj2[i][j] = objr2[i][j];
                }
            //3D array
            for (int i = 0; i < objr3.length; i++)
                for(int j = 0; j < objr3[i].length; j++)
                    for(int k = 0; k < objr3[i][j].length; k++)
                    {
                        tmp_obj3[i][j][k] = objr3[i][j][k];
                    }
            //Compare the last value of the array correctly
            if(tmp_obj1[3] == (Object)'d' && tmp_obj2[2][0] == (Object)'g' && tmp_obj3[1][2][0] == (Object)'n')
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
            Object [] tmp_obj1 = new Object[4];
            Object [][] tmp_obj2 = new Object [3][4];
            Object [][][] tmp_obj3 = new Object [2][3][4];
            System.arraycopy(objr1,0,tmp_obj1,0,objr1.length);
            System.arraycopy(objr2,0,tmp_obj2,0,objr2.length);
            System.arraycopy(objr3,0,tmp_obj3,0,objr3.length);
            //Compare the last value of the array correctly
            if(tmp_obj1[3] == (Object)'d' && tmp_obj2[2][0] == (Object)'g' && tmp_obj3[1][2][0] == (Object)'n')
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
            Object [] tmp_obj1 = Arrays.copyOf(objr1,objr1.length);
            Object [][] tmp_obj2 = Arrays.copyOf(objr2,objr2.length);
            Object [][][] tmp_obj3 = Arrays.copyOf(objr3,objr3.length);
            //Compare the last value of the array correctly
            if(tmp_obj1[3] == (Object)'d' && tmp_obj2[2][0] == (Object)'g' && tmp_obj3[1][2][0] == (Object)'n')
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
            Object [] tmp_obj1 = Arrays.copyOfRange(objr1,0,objr1.length);
            Object [][] tmp_obj2 = Arrays.copyOfRange(objr2,0,objr2.length);
            Object [][][] tmp_obj3 = Arrays.copyOfRange(objr3,0,objr3.length);
            //Compare the last value of the array correctly
            if(tmp_obj1[3] == (Object)'d' && tmp_obj2[2][0] == (Object)'g' && tmp_obj3[1][2][0] == (Object)'n')
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
            Object [] tmp_obj1 = objr1.clone();
            Object [][] tmp_obj2 = objr2.clone();
            Object [][][] tmp_obj3 = objr3.clone();
            //Compare the last value of the array correctly
            if(tmp_obj1[3] == (Object)'d' && tmp_obj2[2][0] == (Object)'g' && tmp_obj3[1][2][0] == (Object)'n')
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
        private void method06(Object[] arr_obj1, Object[][] arr_obj2, Object[][][] arr_obj3)
        {
            Object[] tmp_obj1 = {'b','s','d','g'};
            Object[][] tmp_obj2 = {{'b','s','d','g'},{'e','f'},{'s'}};
            Object[][][] tmp_obj3 = {{{'b','s','d','g'},{'e','f'},{'s'}},{{'h','i','j','k'},{'l','m'},{'a'}}};
            //Check the values defined in the function
            if(tmp_obj1[3] == (Object)'g' && tmp_obj2[2][0] == (Object)'s' && tmp_obj3[1][2][0] == (Object)'a')
                incCheckCount();
            else
                System.out.println("ErrorResult in test06 step1");
            //Check unmodified parameter values
            if(arr_obj1[3] == (Object)'d' && arr_obj2[2][0] == (Object)'g'  && arr_obj3[1][2][0] == (Object)'n')
                incCheckCount();
            else
                System.out.println("ErrorResult in test06 step2");
        }
        public void run()
        {
            method06(objr1,objr2,objr3);
        }
    }

    private class test07 implements Runnable{
        /*test07 Interface call, call function change to modify the parameter value and judge*/
        private Object change(Object temp1, Object temp2){
            temp1 = temp2;
            return temp1;
        }

        private void method07(Object[] arr_obj1, Object[][] arr_obj2, Object[][][] arr_obj3) {
            Object[] objr1 = {'b','s','d','g'};
            Object[][] objr2 = {{'b','s','d','g'},{'e','f'},{'s'}};
            Object[][][] objr3 = {{{'b','s','d','g'},{'e','f'},{'s'}},{{'h','i','j','k'},{'l','m'},{'a'}}};
            arr_obj1 = (Object[]) change(arr_obj1,objr1);
            arr_obj2 = (Object[][])change(arr_obj2,objr2);
            arr_obj3 = (Object[][][]) change(arr_obj3,objr3);
            //Check the values defined in the function
            if(objr1[3] == (Object)'g' && objr2[2][0] == (Object)'s' && objr3[1][2][0] == (Object)'a')
                incCheckCount();
            else
                System.out.println("ErrorResult in test07 step1");
            //Check the modified parameter values
            if(arr_obj1[3] == (Object)'g' && arr_obj2[2][0] == (Object)'s'  && arr_obj3[1][2][0] == (Object)'a')
                incCheckCount();
            else
                System.out.println("ErrorResult in test07 step2");
        }
        public void run()
        {
            method07(objr1,objr2,objr3);
        }
    }

    private class test08 implements Runnable{
        /*Call the no-argument constructor, initialize the variables of the parent class, and assign values to the
        fields of the newly created object, and judge the result*/
        private void method08() {
            final Object [] VALUE4 = RC_Array_final04();
            final Object[][] VALUE5 = RC_Array_final05();
            final Object[][][] VALUE6 = RC_Array_final06();
            RC_Array_12 rctest = new RC_Array_12();
            rctest.obj_base1 = VALUE4;
            rctest.obj_base2 = VALUE5;
            rctest.obj_base3 = VALUE6;
            //Check the values defined in the function
            if(VALUE4[3] == (Object)'g' && VALUE5[2][0] == (Object)'s' && VALUE6[1][2][0] == (Object)'a')
                incCheckCount();
            else
                System.out.println("ErrorResult in test08 step1");
            //Check the modified parameter values
            if(rctest.obj_base1[3] == (Object)'g' && rctest.obj_base2[2][0] == (Object)'s'  && rctest.obj_base3[1][2][0] == (Object)'a')
                incCheckCount();
            else
                System.out.println("ErrorResult in test08 step2");

        }
        public void run()
        {
            method08();
        }
    }

    private class test09 implements Runnable{
        /*Exception test*/
        private void method09(){
            int check = 0;
            Object [] value1 = RC_Array_final04();
            Object[][] value2 = RC_Array_final05();
            Object[][][] value3 = RC_Array_final06();
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
