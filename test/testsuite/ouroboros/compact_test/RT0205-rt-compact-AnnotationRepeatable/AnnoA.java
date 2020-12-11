import java.lang.annotation.*;


@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@Repeatable(Test.class)
@AnnoB(intB = 1)
@AnnoB(intB = 2)
@AnnoB(intB = 3)
@AnnoB(intB = 4)
@AnnoB(intB = 5)
@AnnoB(intB = 6)
@AnnoB(intB = 7)
@AnnoB(intB = 8)
@AnnoB(intB = 9)
@AnnoB(intB = 10)
@AnnoB(intB = 11)
@AnnoB(intB = 12)
@AnnoB(intB = 13)
@AnnoB(intB = 14)
@AnnoB(intB = 15)
@AnnoB(intB = 16)
@AnnoB(intB = 17)
@AnnoB(intB = 18)
public @interface AnnoA {
    int intA() default 0;
    byte byteA() default 0;
    char charA() default 0;
    double doubleA() default 0;
    boolean booleanA() default true;
    long longA() default 0;
    float floatA() default 0;
    short shortA() default 0;
    int[] intAA() default 0;
    byte[] byteAA() default 0;
    char[] charAA() default 0;
    double[] doubleAA() default 0;
    boolean[] booleanAA() default true;
    long[] longAA() default 0;
    float[] floatAA() default 0;
    short[] shortAA() default 0;
    String stringA() default "";
    String[] stringAA() default "";
    Class classA() default Thread.class;
    Class[] classAA() default Thread.class;
    Thread.State stateA() default Thread.State.BLOCKED;
    Thread.State[] stateAA() default Thread.State.BLOCKED;
    AnnoB annoBA() default @AnnoB;
    AnnoB[] annoBAA() default @AnnoB;
    AnnoB[] value();
}
