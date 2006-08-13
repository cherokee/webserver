/* tdate_parse - parse string dates into internal form, stripped-down version
**
** Copyright © 1995 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/* This is a stripped-down / modified version of date_parse.c, available at
** http://www.acme.com/software/date_parse/
*/

#include <sys/types.h>

#include <ctype.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tdate_parse.h"


struct strlong {
	   char* s;
	   long l;
};


static void
pound_case( char* str )
{
	   for ( ; *str != '\0'; ++str )
	   {
			 if ( isupper( (int) *str ) )
				    *str = tolower( (int) *str );
	   }
}


static int
strlong_search( char* str, struct strlong* tab, int n, long* lP )
{
	   int i, h, l, r;

	   l = 0;
	   h = n - 1;
	   for (;;)
	   {
			 i = ( h + l ) / 2;
			 r = strcmp( str, tab[i].s );
			 if ( r < 0 )
				    h = i - 1;
			 else if ( r > 0 )
				    l = i + 1;
			 else
			 {
				    *lP = tab[i].l;
				    return 1;
			 }
			 if ( h < l )
				    return 0;
	   }
}


static int
scan_wday( char* str_wday, long* tm_wdayP )
{
	   static struct strlong wday_tab[] = {
			{ "fri",       5 },
			{ "friday",    5 },
			{ "mon",       1 },
			{ "monday",    1 },
			{ "sat",       6 },
			{ "saturday",  6 },
			{ "sun",       0 },
			{ "sunday",    0 },
			{ "thu",       4 },
			{ "thursday",  4 },
			{ "tue",       2 },
			{ "tuesday",   2 },
			{ "wed",       3 },
			{ "wednesday", 3 }
		};

	   pound_case( str_wday );
	   return strlong_search(
		str_wday, wday_tab, sizeof(wday_tab)/sizeof(struct strlong), tm_wdayP );
}


static int
scan_mon( char* str_mon, long* tm_monP )
{
	   static struct strlong mon_tab[] = {
			{ "apr",       3 },
			{ "april",     3 },
			{ "aug",       7 },
			{ "august",    7 },
			{ "dec",      11 },
			{ "december", 11 },
			{ "feb",       1 },
			{ "february",  1 },
			{ "jan",       0 },
			{ "january",   0 },
			{ "jul",       6 },
			{ "july",      6 },
			{ "jun",       5 },
			{ "june",      5 },
			{ "mar",       2 },
			{ "march",     2 },
			{ "may",       4 },
			{ "nov",      10 },
			{ "november", 10 },
			{ "oct",       9 },
			{ "october",   9 },
			{ "sep",       8 },
			{ "september", 8 }
		};

	   pound_case( str_mon );
	   return strlong_search(
		str_mon, mon_tab, sizeof(mon_tab)/sizeof(struct strlong), tm_monP );
}


static int
is_leap( int year )
{
	   return year % 400? ( year % 100 ? ( year % 4 ? 0 : 1 ) : 0 ) : 1;
}


/* Basically the same as mktime(). */
static time_t
tm_to_time( struct tm* tmP )
{
	   time_t t;
	   static int monthtab[12] = {
			 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	   /* Years since epoch, converted to days. */
	   t = ( tmP->tm_year - 70 ) * 365;
	   /* Leap days for previous years. */
	   t += ( tmP->tm_year - 69 ) / 4;
	   /* Days for the beginning of this month. */
	   t += monthtab[tmP->tm_mon];
	   /* Leap day for this year. */
	   if ( tmP->tm_mon >= 2 && is_leap( tmP->tm_year + 1900 ) )
			 ++t;
	   /* Days since the beginning of this month. */
	   t += tmP->tm_mday - 1;	/* 1-based field */
	   /* Hours, minutes, and seconds. */
	   t = t * 24 + tmP->tm_hour;
	   t = t * 60 + tmP->tm_min;
	   t = t * 60 + tmP->tm_sec;

	   return t;
}


/*
** Parse the date-time string and returns the equivalent time in seconds.
*/
time_t
tdate_parse( char* str )
{
	   struct tm tm;
	   char* cp;
	   char str_mon[64], str_wday[64];
	   char str_sep1[8], str_sep2[8];
	   int tm_sec, tm_min, tm_hour, tm_mday, tm_year;
	   long tm_mon, tm_wday;
	   time_t t;

	   /* Initialize. */
	   (void) memset( (char*) &tm, 0, sizeof(struct tm) );

	   /* Skip initial whitespace ourselves - sscanf is clumsy at this. */
	   for ( cp = str; *cp == ' ' || *cp == '\t'; ++cp )
			 continue;

	   /* If there is no date-time, then return now */
	   if ( !*cp )
		    return (time_t) -1;

	   /* And do the sscanfs.  WARNING: you can add more formats here,
	   ** but be careful!  You can easily screw up the parsing of existing
	   ** formats when you add new ones.  The order is important.
	   */

	   /* wdy, DD mth YYYY HH:MM:SS GMT (RFC 822, 1123 - standard). */
	   if ( sscanf( cp, "%60[a-zA-Z], %d %60[a-zA-Z] %d %d:%d:%d GMT",
			str_wday, &tm_mday, str_mon, &tm_year,
			&tm_hour, &tm_min, &tm_sec ) == 7 &&
		    scan_wday( str_wday, &tm_wday ) &&
		    scan_mon( str_mon, &tm_mon ) )
	   {
			tm.tm_wday = tm_wday;
			tm.tm_mday = tm_mday;
			tm.tm_mon = tm_mon;
			tm.tm_year = tm_year;
			tm.tm_hour = tm_hour;
			tm.tm_min = tm_min;
			tm.tm_sec = tm_sec;
	   }

	   /* wdy, DD-mth-YY HH:MM:SS GMT  (RFC 850 - obsolete) */
	   /* wdy, DD-mth-YYYY HH:MM:SS GMT (strange RFC 822) */
	   else
	   if ( sscanf( cp, "%60[a-zA-Z], %d%2[ -]%60[a-zA-Z]%2[ -]%d %d:%d:%d GMT",
			str_wday, &tm_mday, str_sep1, str_mon, str_sep2, &tm_year,
			&tm_hour, &tm_min, &tm_sec ) == 9 &&
		    scan_wday( str_wday, &tm_wday ) &&
		    scan_mon( str_mon, &tm_mon ) )
	   {
			tm.tm_wday = tm_wday;
			tm.tm_mday = tm_mday;
			tm.tm_mon = tm_mon;
			tm.tm_year = tm_year;
			tm.tm_hour = tm_hour;
			tm.tm_min = tm_min;
			tm.tm_sec = tm_sec;
	   }

	   /* wdy mth DD HH:MM:SS YYYY (ANSI C asctime() format - uncommon) */
	   else
	   if ( sscanf( cp, "%60[a-zA-Z] %60[a-zA-Z] %d %d:%d:%d %d",
			str_wday, str_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec,
			&tm_year ) == 7 &&
		    scan_wday( str_wday, &tm_wday ) &&
		    scan_mon( str_mon, &tm_mon ) )
	   {
			tm.tm_wday = tm_wday;
			tm.tm_mon = tm_mon;
			tm.tm_mday = tm_mday;
			tm.tm_hour = tm_hour;
			tm.tm_min = tm_min;
			tm.tm_sec = tm_sec;
			tm.tm_year = tm_year;
	   }
	   /* other VERY VERY UNCOMMON formats */

	   /* wdy mth DD HH:MM:SS GMT YY */
	   else
	   if ( sscanf( cp, "%60[a-zA-Z] %60[a-zA-Z] %d %d:%d:%d GMT %d",
			str_wday, str_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec,
			&tm_year ) == 7 &&
		    scan_wday( str_wday, &tm_wday ) &&
		    scan_mon( str_mon, &tm_mon ) )
	   {
			tm.tm_wday = tm_wday;
			tm.tm_mon = tm_mon;
			tm.tm_mday = tm_mday;
			tm.tm_hour = tm_hour;
			tm.tm_min = tm_min;
			tm.tm_sec = tm_sec;
			tm.tm_year = tm_year;
	   }

	   /* DD-mth-YY HH:MM:SS GMT */
	   /* DD mth YY HH:MM:SS GMT */
	   else
	   if ( sscanf( cp, "%d%2[ -]%60[a-zA-Z]%2[ -]%d %d:%d:%d GMT",
			&tm_mday, str_sep1, str_mon, str_sep2, &tm_year,
			&tm_hour, &tm_min, &tm_sec ) == 8 &&
		    scan_mon( str_mon, &tm_mon ) )
	   {
			tm.tm_mday = tm_mday;
			tm.tm_mon = tm_mon;
			tm.tm_year = tm_year;
			tm.tm_hour = tm_hour;
			tm.tm_min = tm_min;
			tm.tm_sec = tm_sec;
	   }

	   /* HH:MM:SS GMT DD-mth-YY */
	   /* HH:MM:SS GMT DD mth YY */
	   else
	   if ( sscanf( cp, "%d:%d:%d GMT %d%2[ -]%60[a-zA-Z]%2[ -]%d",
			&tm_hour, &tm_min, &tm_sec,
			&tm_mday, str_sep1, str_mon, str_sep2, &tm_year ) == 8 &&
		    scan_mon( str_mon, &tm_mon ) )
	   {
			tm.tm_hour = tm_hour;
			tm.tm_min = tm_min;
			tm.tm_sec = tm_sec;
			tm.tm_mday = tm_mday;
			tm.tm_mon = tm_mon;
			tm.tm_year = tm_year;
	   }
	   else
			return (time_t) -1;

	   if ( tm.tm_year > 1900 )
			 tm.tm_year -= 1900;
	   else if ( tm.tm_year < 70 )
			 tm.tm_year += 100;

	   t = tm_to_time( &tm );

	   return t;
}
