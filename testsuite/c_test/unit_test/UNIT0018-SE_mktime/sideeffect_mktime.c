#include <stdio.h>      /* printf, scanf */
#include <time.h>       /* time_t, struct tm, time, mktime */

int main ()
{
    time_t rawtime;
    struct tm timeinfo;
    int year, month ,day;
    const char * weekday[] = { "Sunday", "Monday","Tuesday", "Wednesday","Thursday", "Friday", "Saturday"};

    timeinfo.tm_sec = 0;
    timeinfo.tm_min = 1;
    timeinfo.tm_hour = 1;
    timeinfo.tm_mday = 10;
    timeinfo.tm_mon = 7;
    timeinfo.tm_year = 95;
    timeinfo.tm_isdst = -1;

    mktime ( &timeinfo );
    printf ("mktime set tm_wday is %s\n", weekday[timeinfo.tm_wday]);

    timeinfo.tm_wday = 3;
    // may modify timeinfo
    mktime ( &timeinfo );
    printf ("mktime set tm_wday is %s\n", weekday[timeinfo.tm_wday]);
    return 0;
}
