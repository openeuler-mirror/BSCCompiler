import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@AnnoA( value = @AnnoB(intB = -1))
@AnnoA( value = {@AnnoB(intB = -2), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -3), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -4), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -5), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -6), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -7), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -8), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -9), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
@AnnoA( value = {@AnnoB(intB = -10), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1), @AnnoB(intB = 1)})
public @interface Test{
    AnnoA[] value();
}
