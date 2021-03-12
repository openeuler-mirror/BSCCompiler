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
import java.lang.annotation.Annotation;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Field;
import java.lang.reflect.Type;
public class FieldNativeUncover {
	private static int res = 99;
	public String userName;
	public boolean flag;
	public byte newByte;
	public char chars;
	public short shorts;
	public int num;
	public long longs;
	public float floats;
	@newAnnotation(name = "double1", value = "double2")
	public double doubles;
	@Retention(RetentionPolicy.RUNTIME)
	public @interface newAnnotation{
		public String name();
		public String value();
	}
	public static void main(String[] args) {
		int  result = 2; 
		FieldDmeo1();
		if(result == 2 && res == 71) {
			res = 0;
		}
		System.out.println(res);
	}
	public static void FieldDmeo1() {
		FieldNativeUncover fieldNativeUncover = new FieldNativeUncover();
		test1(fieldNativeUncover);
		test2(fieldNativeUncover);
		test3(fieldNativeUncover);
		test4(fieldNativeUncover);
		test5(fieldNativeUncover);
		test6(fieldNativeUncover);
		test7(fieldNativeUncover);
		test8(fieldNativeUncover);
		test9(fieldNativeUncover);
		test10(fieldNativeUncover);
		test11(fieldNativeUncover);
		test12(fieldNativeUncover);
		test13(fieldNativeUncover);
		test14(fieldNativeUncover);
	}
	/**
	 * private native String getNameInternal();
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test1(FieldNativeUncover fieldNativeUncover) {
			try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("userName");
			String name = field.getName();//getNameInternal() called by getName();
			if(name.equals("userName")) {
			//System.out.println(name);
			FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
			}catch( NoSuchFieldException e) {
				e.printStackTrace();
			}
		return true;
	}
	/**
	 * private native String[] getSignatureAnnotation();
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test2(FieldNativeUncover fieldNativeUncover) {
			try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("userName");
			Type type = field.getGenericType();//getSignatureAttribute() called by getGenericType(),getSignatureAnnotation() called by getSignatureAttribute();
			if(type.toString().equals("class java.lang.String")) {
				//System.out.println(type.toString());
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
			}catch( NoSuchFieldException e) {
				e.printStackTrace();
			}
		return true;
	}
	/**
	 * public native Object get(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test3(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("userName");
			field.set(fieldNativeUncover, "Tom");// set the attribute "userName" as  "Tom";
			Object object = field.get(fieldNativeUncover);
			if(object.toString().equals("Tom")) {
			//System.out.println(object);
			FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native boolean getBoolean(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test4(FieldNativeUncover fieldNativeUncover){
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("flag");
			field.setBoolean(fieldNativeUncover, true);
			boolean flags = field.getBoolean(fieldNativeUncover);
			if(flags) {
			//System.out.println(flags);
			FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native byte getByte(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test5(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("newByte");
			field.setByte(fieldNativeUncover, (byte) 100);
			byte flags = field.getByte(fieldNativeUncover);
			if(flags == 100) {
				//System.out.println(flags);
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native char getChar(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test6(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("chars");
			field.setChar(fieldNativeUncover, 'a');
			char chars = field.getChar(fieldNativeUncover);
			if ("a".equals(String.valueOf(chars))) {
				//System.out.println(chars);
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native short getShort(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test7(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("shorts");
			field.setShort(fieldNativeUncover, (short) 20);
			short sh = field.getShort(fieldNativeUncover);
			if (sh==20) {
				//System.out.println(sh);
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native int getInt(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test8(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("num");
			field.setInt(fieldNativeUncover, 20);
			int number = field.getInt(fieldNativeUncover);
			if (number == 20) {
				//System.out.println(number);
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native long getLong(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test9(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("longs");
			field.setLong(fieldNativeUncover, 20);
			long number = field.getLong(fieldNativeUncover);
			if (number == 20) {
				//System.out.println(number);
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native float getFloat(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test10(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("floats");
			field.setFloat(fieldNativeUncover, (float) 1.0);
			Float number = field.getFloat(fieldNativeUncover);
			if (number==1.0) {
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * public native double getDouble(Object obj)throws IllegalArgumentException, IllegalAccessException;
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test11(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("doubles");
			field.setDouble(fieldNativeUncover,1.05);
			double number = field.getDouble(fieldNativeUncover);
			if (number==1.05) {
				FieldNativeUncover.res = FieldNativeUncover.res - 2;
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * private native <A extends Annotation> A getAnnotationNative(Class<A> annotationType);
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test12(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("doubles");
			newAnnotation annotation = field.getAnnotation(newAnnotation.class);//getAnnotationNative() called by getAnnotation()
			if(annotation.toString().equals("@FieldNativeUncover$newAnnotation(name=double1, value=double2)")) {
			//System.out.println(annotation.toString());
			FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
	/**
	 * private native boolean isAnnotationPresentNative(Class<? extends Annotation> annotationType);
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test13(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("doubles");
			newAnnotation[] annotations = field.getAnnotationsByType(newAnnotation.class);//isAnnotationPresentNative() called by getAnnotationsByType()
			if(annotations.length == 1) {
			//System.out.println(annotations.length);
			FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		return true;
		}
		return true;
	}
	/**
	 * public native Annotation[] getDeclaredAnnotations();
	 * @param fieldNativeUncover
	 * @return
	*/

	public static boolean test14(FieldNativeUncover fieldNativeUncover) {
		try {
			Class class1 = fieldNativeUncover.getClass();
			Field field = class1.getField("doubles");
			Annotation[] annotations =field.getDeclaredAnnotations();
			if(annotations.length == 1 && annotations.getClass().toString().equals("class [Ljava.lang.annotation.Annotation;")) {
			//System.out.println(annotations.length);
			//System.out.println(annotations.getClass().toString());
			FieldNativeUncover.res = FieldNativeUncover.res - 2;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
	}
}
