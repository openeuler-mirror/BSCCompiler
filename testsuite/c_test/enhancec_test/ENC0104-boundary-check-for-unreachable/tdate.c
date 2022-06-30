/*
 * (c) Copyright 2015-2020 by Solid Sands B.V.,
 *     Amsterdam, the Netherlands. All rights reserved.
 *     Subject to conditions in the RESTRICTIONS file.
 * (c) Copyright 2003-2006 ACE Associated Compiler Experts bv
 * (c) Copyright 1985,2003-2006 ACE Associated Computer Experts bv
 * All rights reserved.  Subject to conditions in RESTRICTIONS file.
 */

/*
 *      This test tests respectively the month_day, month_name and
 *      the day_of_year function, and some versions of them.
 *      These functions are taken from the Brian W. Kernighan &
 *      Dennis M. Ritchie C-programming language book.
 */

#include "def.h"

static int day_tab[2][13] = {
  {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
struct date {
  int day ;
  int month ;
  int year ;
  int yearday ;
};

int
day1_of_year(int year, int month, int day) /* set day of year */
{
  int i, leap;

  leap = year%4 == 0 && year%100 != 0 || year%400 == 0;
  for (i = 1; i < month; i++)
    day += day_tab[leap][i];
  return(day);
}

void
month1_day(int year, int yearday, int *pmonth, int *pday) /* set month, day */
{
  int i, leap;

  leap = year%4 == 0 && year%100 != 0 || year%400 == 0;
  for (i = 1; yearday > day_tab[leap][i]; i++)
    yearday -= day_tab[leap][i];
  *pmonth = i;
  *pday = yearday;
}

const char *
month_name(int n) /* return name of n-th month */
{
  static const char * name[] = {
    "illegal month",
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
  };

  return((n < 1 || n > 12) ? name[0] : name[n]);
}

int
day2_of_year(struct date *pd) /* set day of year from month, day */
{
  int i, day, leap;

  day = pd->day;
  leap = pd->year % 4 == 0 && pd->year % 100 != 0
         || pd->year % 400 == 0;
  for (i = 0; i < pd->month; i++)
    day += day_tab[leap][i];
  return(day);
}

void
month2_day(struct date *pd) /* set month and day from day of year */
{
  int i, leap;

  leap = pd->year % 4 == 0 && pd->year % 100 != 0
         || pd->year % 400 == 0;
  pd->day = pd->yearday;
  for (i = 1; pd->day > day_tab[leap][i]; i++)
    pd->day -= day_tab[leap][i];
  pd->month = i;
}

int month_global;
int day_global;
struct date date_global;

MAIN
{
  int result;
  const char *name;
  int *dayptr, *monptr;
  struct date *dateptr;

  CVAL_HEADER("functions that have something to do with date");

  CVAL_SECTION("month_name function" );
  CVAL_DOTEST( name = month_name( 13 )) ;
  CVAL_VERIFY(!(cval_strcmp( name, "illegal month" )));

  CVAL_DOTEST( name = month_name( 1 )) ;
  CVAL_VERIFY(!(cval_strcmp( name, "January" )));

  CVAL_DOTEST( name = month_name( 12 )) ;
  CVAL_VERIFY(!(cval_strcmp( name, "December" )));


  CVAL_ENDSECTION();

  CVAL_SECTION("day_of_year function" );
  CVAL_DOTEST( result = day1_of_year(1985, 10, 23)) ;
  CVAL_VERIFY( result == 296);

  CVAL_DOTEST( result = day1_of_year(1985, 12, 31)) ;
  CVAL_VERIFY( result == 365);

  CVAL_DOTEST( result = day1_of_year(1988, 12, 31)) ;
  CVAL_VERIFY( result == 366);


  CVAL_ENDSECTION();

  CVAL_SECTION("month_day function" );
  monptr = &month_global ;
  dayptr = &day_global;
  CVAL_DOTEST( month1_day(1985,296,monptr,dayptr)) ;
  CVAL_VERIFY( *monptr == 10 && *dayptr == 23);

  CVAL_DOTEST( month1_day(1988,296,monptr,dayptr)) ;
  CVAL_VERIFY( *monptr == 10 && *dayptr == 22);


  CVAL_ENDSECTION();

  CVAL_SECTION("day_of_year pointer version" );
  dateptr = &date_global;
  dateptr->day = 23 ;
  dateptr->year = 1985 ;
  dateptr->month = 10 ;
  CVAL_DOTEST( result = day2_of_year( dateptr )) ;
  CVAL_VERIFY( result == 296);

  dateptr->day = 31 ;
  dateptr->year = 1985 ;
  dateptr->month = 12 ;
  CVAL_DOTEST( result = day2_of_year( dateptr )) ;
  CVAL_VERIFY( result == 365);

  dateptr->day = 31 ;
  dateptr->year = 1988 ;
  dateptr->month = 12 ;
  CVAL_DOTEST( result = day2_of_year( dateptr )) ;
  CVAL_VERIFY( result == 366);


  CVAL_ENDSECTION();

  CVAL_SECTION("month_day pointer version" );
  dateptr->year = 1985 ;
  dateptr->yearday = 60 ;
  CVAL_DOTEST( month2_day( dateptr )) ;
  CVAL_VERIFY((dateptr->month == 3) && (dateptr->day == 1));

  dateptr->year = 1988 ;
  dateptr->yearday = 60 ;
  CVAL_DOTEST( month2_day( dateptr )) ;
  CVAL_VERIFY((dateptr->month == 2) && (dateptr->day == 29));


  CVAL_ENDSECTION();
}
