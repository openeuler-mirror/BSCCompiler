import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@Repeatable(AnnoA.class)
public @interface AnnoB {
    int intB() default 999;
}
