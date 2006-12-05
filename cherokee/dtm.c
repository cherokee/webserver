/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      A.D.F.              <adefacc@tin.it>
 *
 * Copyright (C) 2006 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "common-internal.h"

#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "dtm.h"

typedef struct RecNameLen_s {
	const char  *name1;		/* first name */
	size_t       len1;		/* first name length */
	const char  *name2;		/* second name */
	size_t       len2;		/* second name length */
} RecNameLen_t;


/*
** Returns TRUE if it matches an existing weekday name.
*/
static int
cvt_wday_name2idx( const char* str_wday, size_t len_wday, int* ptm_wday )
{
	static RecNameLen_t wday_tab[] = {
	{ "Sun", 3, "Sunday",    6 },
	{ "Mon", 3, "Monday",    6 },
	{ "Tue", 3, "Tuesday",   7 },
	{ "Wed", 3, "Wednesday", 9 },
	{ "Thu", 3, "Thursday",  8 },
	{ "Fri", 3, "Friday",    6 },
	{ "Sat", 3, "Saturday",  8 }
	};

	/* Fast test on min. length.
	*/
	if (len_wday < 3)
		return 0;

	/* Fast guess of day of week.
	*/
	switch( str_wday[0] ) {
		case 's':
		case 'S':
			if ( str_wday[1] == 'u' ||
			     str_wday[1] == 'U')
				*ptm_wday = 0;
			else
				*ptm_wday = 6;
			break;

		case 'm':
		case 'M':
			*ptm_wday = 1;
			break;

		case 't':
		case 'T':
			if ( str_wday[1] == 'u' ||
			     str_wday[1] == 'U')
				*ptm_wday = 2;
			else
				*ptm_wday = 4;
			break;

		case 'w':
		case 'W':
			*ptm_wday = 3;
			break;

		case 'f':
		case 'F':
			*ptm_wday = 5;
			break;

		default:
			return 0;
	}

	/* If length matches the length of short name, then compare the name.
	*/
	if ( len_wday == wday_tab[*ptm_wday].len1 ) {
		return ( strncasecmp(
			str_wday,
			wday_tab[*ptm_wday].name1,
			wday_tab[*ptm_wday].len1 ) == 0 );
	}

	/* If length matches the length of long name, then compare the name.
	*/
	if ( len_wday == wday_tab[*ptm_wday].len2 ) {
		return ( strncasecmp(
			str_wday,
			wday_tab[*ptm_wday].name2,
			wday_tab[*ptm_wday].len2 ) == 0 );
	}

	/* No match.
	*/
	return 0;
}


/* Returns TRUE if it matches an existing month name. */
static int
cvt_mon_name2idx( const char* str_mon, size_t len_mon, int* ptm_mon )
{
	static RecNameLen_t mon_tab[] = {
	{ "Jan", 3, "January",   7 },
	{ "Feb", 3, "February",  8 },
	{ "Mar", 3, "March",     5 },
	{ "Apr", 3, "April",     5 },
	{ "May", 3, "May",       3 },
	{ "Jun", 3, "June",      4 },
	{ "Jul", 3, "July",      4 },
	{ "Aug", 3, "August",    6 },
	{ "Sep", 3, "September", 9 },
	{ "Oct", 3, "October",   7 },
	{ "Nov", 3, "November",  8 },
	{ "Dec", 3, "December",  8 }
	};

	/* First fast test on min. length.
	*/
	if (len_mon < 3)
		return 0;

	/* Fast guess of month name.
	*/
	switch( str_mon[0] ) {
		case 'j':
		case 'J':
			if ( str_mon[1] == 'a' ||
			     str_mon[1] == 'A')
				*ptm_mon = 0;
			else
			if ( str_mon[2] == 'n' ||
			     str_mon[2] == 'N')
				*ptm_mon = 5;
			else
				*ptm_mon = 6;
			break;

		case 'f':
		case 'F':
			*ptm_mon = 1;
			break;

		case 'm':
		case 'M':
			if ( str_mon[2] == 'r' ||
			     str_mon[2] == 'R')
				*ptm_mon = 2;
			else
				*ptm_mon = 4;
			break;

		case 'a':
		case 'A':
			if ( str_mon[2] == 'r' ||
			     str_mon[2] == 'R')
				*ptm_mon = 3;
			else
				*ptm_mon = 7;
			break;

		case 's':
		case 'S':
			*ptm_mon = 8;
			break;

		case 'o':
		case 'O':
			*ptm_mon = 9;
			break;

		case 'n':
		case 'N':
			*ptm_mon = 10;
			break;

		case 'd':
		case 'D':
			*ptm_mon = 11;
			break;

		default:
			return 0;
	}

	/* If length matches the length of short name, then compare the name.
	*/
	if ( len_mon == mon_tab[*ptm_mon].len1 )
	{
		return ( strncasecmp(
			str_mon,
			mon_tab[*ptm_mon].name1,
			mon_tab[*ptm_mon].len1 ) == 0 );
	}

	/* If length matches the length of long name, then compare the name.
	*/
	if ( len_mon == mon_tab[*ptm_mon].len2 )
	{
		return ( strncasecmp(
			str_mon,
			mon_tab[*ptm_mon].name2,
			mon_tab[*ptm_mon].len2 ) == 0 );
	}

	/* No match.
	*/
	return 0;
}


/* HH:MM:SS GMT DD-mth-YY
** DD-mth-YY HH:MM:SS GMT
** Returns TRUE if it matches one of the above date-time formats.
*/
static int
dft_dmyhmsr2tm( char *cp, struct tm *ptm )
{
	size_t idx = 0;

	--idx;
	do {
		++idx;
	}
	while( cp[idx] == ' ' );

	cp += idx;
	idx = 0;

	ptm->tm_wday = 0;

	if ( cp[2] == ':' ) {

		/* HH:MM:SS GMT DD-mth-YY
		*/

		/* hour, min, sec
		*/
		if (
			!isdigit( cp[0] ) || !isdigit( cp[1] ) ||
			cp[2] != ':' ||
			!isdigit( cp[3] ) || !isdigit( cp[4] ) ||
			cp[5] != ':' ||
			!isdigit( cp[6] ) || !isdigit( cp[7] )
			)
			return 0;

		ptm->tm_hour = (cp[0] - '0') * 10 + (cp[1] - '0');
		ptm->tm_min  = (cp[3] - '0') * 10 + (cp[4] - '0');
		ptm->tm_sec  = (cp[6] - '0') * 10 + (cp[7] - '0');

		idx += 8;
		if ( cp[idx] != ' ')
			return 0;
		do {
			++idx;
		}
		while( cp[idx] == ' ' );

		cp += idx;
		idx = 0;

		if ( cp[0] != 'G' ||
		     cp[1] != 'M' ||
		     cp[2] != 'T' ||
		     cp[3] != ' ' )
			return 0;

		idx += 3;
		do {
			++idx;
		}
		while( cp[idx] == ' ' );
		cp += idx;

		/* day
		*/
		ptm->tm_mday = 0;
		for ( idx = 0; idx < 2 && isdigit( cp[idx] ); ++idx ) {
			ptm->tm_mday = ptm->tm_mday * 10  + (cp[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( cp[idx] != '-')
			return 0;

		++idx;
		cp += idx;

		/* month
		*/
		ptm->tm_mon = 0;
		for ( idx = 0; isalpha( cp[idx] ); ++idx )
			;

		if (! cvt_mon_name2idx( cp, idx, &(ptm->tm_mon) ) )
			return 0;

		if ( cp[idx] != '-')
			return 0;

		++idx;
		cp += idx;

		/* year
		*/
		ptm->tm_year = 0;
		for ( idx = 0; idx < 4 && isdigit( cp[idx] ); ++idx ) {
			ptm->tm_year = ptm->tm_year * 10  + (cp[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( isdigit( cp[idx] ) )
			return 0;

	} else {

		/* DD-mth-YY HH:MM:SS GMT
		*/

		/* day
		*/
		ptm->tm_mday = 0;
		for ( idx = 0; idx < 2 && isdigit( cp[idx] ); ++idx ) {
			ptm->tm_mday = ptm->tm_mday * 10  + (cp[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( cp[idx] != '-')
			return 0;

		++idx;
		cp += idx;

		/* month
		*/
		ptm->tm_mon = 0;
		for ( idx = 0; isalpha( cp[idx] ); ++idx )
			;
		if (! cvt_mon_name2idx( cp, idx, &(ptm->tm_mon) ) )
			return 0;

		if ( cp[idx] != '-')
			return 0;
		++idx;
		cp += idx;

		/* year
		*/
		ptm->tm_year = 0;
		for ( idx = 0; idx < 4 && isdigit( cp[idx] ); ++idx ) {
			ptm->tm_year = ptm->tm_year * 10  + (cp[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( cp[idx] != ' ' )
			return 0;

		do {
			++idx;
		}
		while( cp[idx] == ' ' );

		cp += idx;
		idx = 0;

		/* hour, min, sec
		*/
		if (
			!isdigit( cp[0] ) || !isdigit( cp[1] ) ||
			cp[2] != ':' ||
			!isdigit( cp[3] ) || !isdigit( cp[4] ) ||
			cp[5] != ':' ||
			!isdigit( cp[6] ) || !isdigit( cp[7] )
			)
			return 0;

		ptm->tm_hour = (cp[0] - '0') * 10 + (cp[1] - '0');
		ptm->tm_min  = (cp[3] - '0') * 10 + (cp[4] - '0');
		ptm->tm_sec  = (cp[6] - '0') * 10 + (cp[7] - '0');

		idx += 8;
		if ( cp[idx] != ' ')
			return 0;
		do {
			++idx;
		}
		while( cp[idx] == ' ' );
		cp += idx;
		idx = 0;

		if ( cp[0] != 'G' ||
		     cp[1] != 'M' ||
		     cp[2] != 'T' )
			return 0;
		idx += 3;
	}
	return 1;
}


/* is leap year */
#define is_leap(y)	( (y) % 4 == 0 && ( (y) % 100 || (y) % 400 == 0 ) )

/* Basically the same as mktime().
*/
static time_t
cvt_tm2time( struct tm* ptm )
{
	time_t t;
	int tm_year = ptm->tm_year + 1900;

	static int monthtab[12] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};

	/* Years since epoch, converted to days. */
	t = ( ptm->tm_year - 70 ) * 365;

	/* Leap days for previous years. */
	t += ( ptm->tm_year - 69 ) / 4;

	/* Days for the beginning of this month. */
	t += monthtab[ptm->tm_mon];

	/* Leap day for this year. */
	if ( ptm->tm_mon >= 2 && is_leap( tm_year ) )
		++t;

	/* Days since the beginning of this month. */
	t += ptm->tm_mday - 1;	/* 1-based field */

	/* Hours, minutes, and seconds. */
	t = t * 24 + ptm->tm_hour;
	t = t * 60 + ptm->tm_min;
	t = t * 60 + ptm->tm_sec;

	return t;
}


#ifdef DTM_DBG
#define RET_TIME_ERR(n)		return ((time_t) (n))
#else
#define RET_TIME_ERR(n)		return ((time_t) -1)
#endif

/*
** Deformat a date time string from one of the http formats
** (commonly used) to a time_t time.
** (str) is assumed to be a zero terminated string.
*/
time_t
cherokee_dtm_str2time( char* str )
{
	struct tm tm;
	char* cp;
	size_t idx = 0;
#ifdef CHECK_MIN_LEN
	size_t len = 0;
#endif /* CHECK_MIN_LEN */
	int tm_sec = 0;
	int tm_min = 0;
	int tm_hour = 0;
	int tm_wday = 0;
	int tm_mday = 0;
	int tm_mon  = 0;
	int tm_year = 0;
	time_t t;

	/* Zero struct tm.
	*/
	(void) memset( (char*) &tm, 0, sizeof(struct tm) );

	/* Skip initial blank character(s).
	*/
	for ( cp = str; *cp == ' ' || *cp == '\t'; ++cp )
		continue;

#ifdef CHECK_MIN_LEN
	/* Test min. length.
	*/
	len = strlen( cp );
	if ( len < 21 )
		RET_TIME_ERR(-1);
#endif /* CHECK_MIN_LEN */

	/* Guess category of date time.
	*/
	if ( isalpha( *cp ) ) {
		/* wdy[,] ...
		** wdy = short or long name of the day of week.
		*/

		/* deformat day of week
		*/
		tm_wday = 0;
		for ( idx = 0; isalpha( cp[idx] ); ++idx )
			;
		if (! cvt_wday_name2idx( cp, idx, &tm_wday ) )
			RET_TIME_ERR(-2);

		/* Another guess of the type of date time format.
		*/
		if ( cp[idx] == ',' ) {

			/* -----------------------------
			 * wdy, DD mth YYYY HH:MM:SS GMT
			 * wdy, DD-mth-YY HH:MM:SS GMT
			 * -----------------------------
			*/

			/* Skip white spaces.
			*/
			++idx;
			if ( cp[idx] != ' ')
				RET_TIME_ERR(-3);
			do {
				++idx;
			}
			while( cp[idx] == ' ' );
			cp += idx;

			/* Deformat day of month.
			*/
			tm_mday = 0;
			for ( idx = 0; idx < 2 && isdigit( cp[idx] ); ++idx ) {
				tm_mday = tm_mday * 10  + (cp[idx] - '0');
			}
			if ( idx == 0 )
				RET_TIME_ERR(-4);

			/* Skip field separator(s).
			*/
			if ( cp[idx] != ' ' && cp[idx] != '-')
				RET_TIME_ERR(-5);
			do {
				++idx;
			}
			while( cp[idx] == ' ' || cp[idx] == '-' );
			cp += idx;

			/* Deformat month.
			*/
			tm_mon = 0;
			for ( idx = 0; isalpha( cp[idx] ); ++idx )
				;
			if (! cvt_mon_name2idx( cp, idx, &tm_mon ) )
				RET_TIME_ERR(-6);

			/* Skip field separator(s).
			*/
			if ( cp[idx] != ' ' && cp[idx] != '-')
				RET_TIME_ERR(-7);
			do {
				++idx;
			}
			while( cp[idx] == ' ' || cp[idx] == '-' );
			cp += idx;

			/* Deformat year.
			*/
			tm_year = 0;
			for ( idx = 0; idx < 4 && isdigit( cp[idx] ); ++idx ) {
				tm_year = tm_year * 10  + (cp[idx] - '0');
			}
			if ( idx == 0 )
				RET_TIME_ERR(-8);

			/* Skip field separator(s).
			*/
			if ( cp[idx] != ' ')
				RET_TIME_ERR(-9);
			do {
				++idx;
			}
			while( cp[idx] == ' ' );
			cp += idx;
			idx = 0;

			/* Deformat hours, minutes, seconds.
			*/
			if (!isdigit( cp[0] ) || !isdigit( cp[1] ) ||
			    cp[2] != ':' ||
			    !isdigit( cp[3] ) || !isdigit( cp[4] ) ||
			    cp[5] != ':' ||
			    !isdigit( cp[6] ) || !isdigit( cp[7] )
			   ) {
				RET_TIME_ERR(-10);
			}
			tm_hour = (cp[0] - '0') * 10 + (cp[1] - '0');
			tm_min  = (cp[3] - '0') * 10 + (cp[4] - '0');
			tm_sec  = (cp[6] - '0') * 10 + (cp[7] - '0');

			/* Skip field separator(s).
			*/
			idx += 8;
			while( cp[idx] == ' ')
				++idx;
			cp += idx;
			idx = 0;

			/* Time Zone (always Greenwitch Mean Time)
			*/
			if ( cp[0] != 'G' ||
			     cp[1] != 'M' ||
			     cp[2] != 'T') {
				RET_TIME_ERR(-11);
			}

		} else {

			/* --------------------------
			 * wdy mth DD HH:MM:SS YYYY
			 * wdy mth DD HH:MM:SS GMT YY
			 * --------------------------
			*/

			if ( cp[idx] != ' ')
				RET_TIME_ERR(-12);
			do {
				++idx;
			}
			while( cp[idx] == ' ' );
			cp += idx;

			/* Deformat month.
			*/
			tm_mon = 0;
			for ( idx = 0; isalpha( cp[idx] ); ++idx )
				;
			if (! cvt_mon_name2idx( cp, idx, &tm_mon ) )
				RET_TIME_ERR(-13);

			/* Skip field separator(s).
			*/
			if ( cp[idx] != ' ')
				RET_TIME_ERR(-14);
			do {
				++idx;
			}
			while( cp[idx] == ' ');
			cp += idx;

			/* Deformat day of month.
			*/
			tm_mday = 0;
			for ( idx = 0; idx < 2 && isdigit( cp[idx] ); ++idx ) {
				tm_mday = tm_mday * 10  + (cp[idx] - '0');
			}
			if ( idx == 0 )
				RET_TIME_ERR(-15);

			/* Skip field separator(s).
			*/
			if ( cp[idx] != ' ')
				RET_TIME_ERR(-16);
			do {
				++idx;
			}
			while( cp[idx] == ' ' );
			cp += idx;
			idx = 0;

			/* Deformat hours, minutes, seconds.
			*/
			if (
				!isdigit( cp[0] ) || !isdigit( cp[1] ) ||
				cp[2] != ':' ||
				!isdigit( cp[3] ) || !isdigit( cp[4] ) ||
				cp[5] != ':' ||
				!isdigit( cp[6] ) || !isdigit( cp[7] )
			   ) {
				RET_TIME_ERR(-17);
			}
			tm_hour = (cp[0] - '0') * 10 + (cp[1] - '0');
			tm_min  = (cp[3] - '0') * 10 + (cp[4] - '0');
			tm_sec  = (cp[6] - '0') * 10 + (cp[7] - '0');

			/* Skip field separator(s).
			*/
			idx += 8;
			if ( cp[idx] != ' ')
				RET_TIME_ERR(-18);
			do {
				++idx;
			}
			while( cp[idx] == ' ' );
			cp += idx;
			idx = 0;

			/* Optional Time Zone (always Greenwitch Mean Time)
			*/
			if ( cp[0] == 'G' ) {
				if ( cp[1] != 'M' ||
				     cp[2] != 'T' ||
				     cp[3] != ' ' )
					RET_TIME_ERR(-19);
				idx = 3;
				do {
					++idx;
				}
				while ( cp[idx] == ' ' );
				cp += idx;
				idx = 0;
			}
			/* else C asctime() format
			*/

			/* Deformat year.
			*/
			tm_year = 0;
			for ( idx = 0; idx < 4 && isdigit( cp[idx] ); ++idx ) {
				tm_year = tm_year * 10  + (cp[idx] - '0');
			}
			if ( idx == 0 )
				RET_TIME_ERR(-20);

			if ( isdigit( cp[idx] ) )
				RET_TIME_ERR(-21);
			cp += idx;
			idx = 0;

		}
	} else
	if ( isdigit( *cp ) ) {
		/* Uncommon date-time formats
		 * -------------------------- 
		 * HH:MM:SS GMT DD-mth-YY
		 * DD-mth-YY HH:MM:SS GMT
		 * --------------------------
		*/
		if ( !dft_dmyhmsr2tm( cp, &tm ) )
			RET_TIME_ERR(-22);
		tm_sec  = tm.tm_sec;
		tm_min  = tm.tm_min;
		tm_hour = tm.tm_hour;
		tm_mday = tm.tm_mday;
		tm_mon  = tm.tm_mon;
		tm_year = tm.tm_year;
		tm_wday = tm.tm_wday;
	} else {
		/* Bad date or unknown date-time format
		*/
		RET_TIME_ERR(-23);
	}

	if ( tm_year >  1900 )
		tm_year -= 1900;
	else
	if ( tm_year < 70 )
		tm_year += 100;

	/* Test field values
	 * NOTE: time has to be in the range 01-Jan-1970 - 31-Dec-2036.
	*/
	if ( tm_year < 70 || tm_year > 136 ||
	   /*tm_mon  <  0 || tm_mon  >  11 ||*/
	     tm_mday <  1 || tm_mday >  31 ||
	     tm_hour <  0 || tm_hour >  23 ||
	     tm_min  <  0 || tm_min  >  59 ||
	     tm_sec  <  0 || tm_sec  >  59 )
		RET_TIME_ERR(-24);

	/* Assign field values.
	*/
	tm.tm_sec  = tm_sec;
	tm.tm_min  = tm_min;
	tm.tm_hour = tm_hour;
	tm.tm_mday = tm_mday;
	tm.tm_mon  = tm_mon;
	tm.tm_year = tm_year;
	tm.tm_wday = tm_wday;

	/* Convert struct tm to time_t.
	*/
	t = cvt_tm2time( &tm );

	/* Return time.
	*/
	return t;
}

/* EOF */
