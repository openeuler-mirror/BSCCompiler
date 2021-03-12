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
import java.lang.reflect.Array;
public class ArrayNativeUncover{
	private static int res = 99;
	public static void main(String[] args) {
		int result = 2;
		ArrayDemo1();
		if(result == 2 && ArrayNativeUncover.res == 97) {
			result = 0;
			System.out.println(result);
		}
	}
	public static void ArrayDemo1() {
		test1();
	}
	/**
	 * private static native Object createObjectArray(Class<?> componentType, int length) throws NegativeArraySizeException;
	 * @return
	*/

	public static boolean test1() {
			try {
			Object object = Array.newInstance(String.class, 5);//createObjectArray() called by newInstance();
			if(object.getClass().toString().equals("class [Ljava.lang.String;")) {
				ArrayNativeUncover.res =ArrayNativeUncover.res - 2;
				return true;
				}
			}catch(Exception e) {
				e.printStackTrace();
			}
		return true;
	}
}
