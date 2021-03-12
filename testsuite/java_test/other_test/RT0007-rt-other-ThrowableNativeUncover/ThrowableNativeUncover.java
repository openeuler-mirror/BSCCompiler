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
public class ThrowableNativeUncover {
	private static int res = 99;
	public static void main(String[] args) {
		ThrowableDemo1();
	}
	public static void ThrowableDemo1() {
		int result = 2;
		Throwable throwable = new Throwable();
		test(throwable);
		test1(throwable);
		if(result == 2 && res == 95) {
			res = 0;
		}
		System.out.println(res);
	}
	/**
	 * private static native Object nativeFillInStackTrace();
	 * @param throwable
	 * @return
	*/

	public static boolean test(Throwable throwable) {
		try {
			Throwable throwable2 = throwable.fillInStackTrace();//nativeFillInStackTrace() called by fillInStackTrace();
			if(throwable2.toString().equals("java.lang.Throwable")) {
			//System.out.println(throwable2.toString());
			ThrowableNativeUncover.res = ThrowableNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;	
	}
	/**
	 * private static native StackTraceElement[] nativeGetStackTrace(Object stackState);
	 * @param throwable
	 * @return
	*/

	public static boolean test1(Throwable throwable) {
		try {
			StackTraceElement[] stackTraceElements = throwable.getStackTrace();//nativeGetStackTrace() called by getStackTrace();
			if(stackTraceElements.length == 3 && stackTraceElements.getClass().toString().equals("class [Ljava.lang.StackTraceElement;")) {
			//System.out.println(stackTraceElements.length);
			//System.out.println(stackTraceElements.getClass().toString());
			ThrowableNativeUncover.res = ThrowableNativeUncover.res - 2;
			}
		}catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}	
}
