/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
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


/* Local macro.
*/
#define is_leap(y) ( (y) % 4 == 0 && ( (y) % 100 || (y) % 400 == 0 ) )


/* Local structs and types.
 */
typedef struct DtmNameLen_s {
	const char  *name1; /* first name  (short, 3 letters) */
	size_t       len1;  /* first name length */
	const char  *name2; /* second name (long) */
	size_t       len2;  /* second name length */
} DtmNameLen_t;


/* Weekday names (short and long names).
 */
static DtmNameLen_t wday_name_tab[] = {
	{ "Sun", 3, "Sunday",    6 },
	{ "Mon", 3, "Monday",    6 },
	{ "Tue", 3, "Tuesday",   7 },
	{ "Wed", 3, "Wednesday", 9 },
	{ "Thu", 3, "Thursday",  8 },
	{ "Fri", 3, "Friday",    6 },
	{ "Sat", 3, "Saturday",  8 }
};


/* Month names (short and long names).
 */
static DtmNameLen_t month_name_tab[] = {
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


/* Number of days in each month (non leap year).
 */
static const int month_days_tab[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


/* Number of days in the years for each month (non leap year).
 */
static const int month_ydays_tab[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};


/* Given the weekday index (...6),
 * it returns the short name of the weekday.
 */
const char *
cherokee_dtm_wday_name(int idxName)
{
	if (idxName < 0 || idxName > 6)
		return "???";
	return wday_name_tab[idxName].name1;
}


/* Given the index of the month (0...11),
 * it returns the short name of the month.
 */
const char *
cherokee_dtm_month_name(int idxName)
{
	if (idxName < 0 || idxName > 11)
		return "???";
	return month_name_tab[idxName].name1;
}


/*
 * Returns TRUE if it matches an existing weekday name.
 */
static int
cvt_wday_name2idx( const char* str_wday, size_t len_wday, struct tm *ptm )
{
	int tm_wday = 0;

	/* Fast test on min. length.
	 */
	if (len_wday < 3)
		return 0;

	/* Fast guess of day of week (Sunday ... Saturday).
	 */
	switch( str_wday[0] ) {
		case 's':
		case 'S':
			if ( str_wday[1] == 'u' ||
			     str_wday[1] == 'U')
				tm_wday = 0;
			else
				tm_wday = 6;
			break;

		case 'm':
		case 'M':
			tm_wday = 1;
			break;

		case 't':
		case 'T':
			if ( str_wday[1] == 'u' ||
			     str_wday[1] == 'U')
				tm_wday = 2;
			else
				tm_wday = 4;
			break;

		case 'w':
		case 'W':
			tm_wday = 3;
			break;

		case 'f':
		case 'F':
			tm_wday = 5;
			break;

		default:
			return 0;
	}
	/* Assign the value.
	 */
	ptm->tm_wday = tm_wday;

	/* If length matches the length of short name, then compare the name.
	 */
	if ( len_wday == wday_name_tab[tm_wday].len1 ) {
		return ( strncasecmp(
		          str_wday,
		          wday_name_tab[tm_wday].name1,
		          wday_name_tab[tm_wday].len1 ) == 0 );
	}

	/* If length matches the length of long name, then compare the name.
	 */
	if ( len_wday == wday_name_tab[tm_wday].len2 ) {
		return ( strncasecmp(
		          str_wday,
		          wday_name_tab[tm_wday].name2,
		          wday_name_tab[tm_wday].len2 ) == 0 );
	}

	/* No match.
	 */
	return 0;
}


/* Returns TRUE if it matches an existing month name. */
static int
cvt_mon_name2idx( const char* str_mon, size_t len_mon, struct tm *ptm )
{
	int tm_mon = 0;

	/* First fast test on min. length.
	 */
	if (len_mon < 3)
		return 0;

	/* Fast guess of month name (January ... December).
	 */
	switch( str_mon[0] ) {
		case 'j':
		case 'J':
			if ( str_mon[1] == 'a' ||
			     str_mon[1] == 'A')
				tm_mon = 0;
			else
			if ( str_mon[2] == 'n' ||
			     str_mon[2] == 'N')
				tm_mon = 5;
			else
				tm_mon = 6;
			break;

		case 'f':
		case 'F':
			tm_mon = 1;
			break;

		case 'm':
		case 'M':
			if ( str_mon[2] == 'r' ||
			     str_mon[2] == 'R')
				tm_mon = 2;
			else
				tm_mon = 4;
			break;

		case 'a':
		case 'A':
			if ( str_mon[2] == 'r' ||
			     str_mon[2] == 'R')
				tm_mon = 3;
			else
				tm_mon = 7;
			break;

		case 's':
		case 'S':
			tm_mon = 8;
			break;

		case 'o':
		case 'O':
			tm_mon = 9;
			break;

		case 'n':
		case 'N':
			tm_mon = 10;
			break;

		case 'd':
		case 'D':
			tm_mon = 11;
			break;

		default:
			return 0;
	}
	/* Assign the value.
	 */
	ptm->tm_mon = tm_mon;

	/* If length matches the length of short name, then compare the name.
	 */
	if ( len_mon == month_name_tab[tm_mon].len1 )
	{
		return ( strncasecmp(
		          str_mon,
		          month_name_tab[tm_mon].name1,
		          month_name_tab[tm_mon].len1 ) == 0 );
	}

	/* If length matches the length of long name, then compare the name.
	 */
	if ( len_mon == month_name_tab[tm_mon].len2 )
	{
		return ( strncasecmp(
		          str_mon,
		          month_name_tab[tm_mon].name2,
		          month_name_tab[tm_mon].len2 ) == 0 );
	}

	/* No match.
	*/
	return 0;
}


/* HH:MM:SS GMT DD-mth-YY
 * DD-mth-YY HH:MM:SS GMT
 * Returns TRUE if it matches one of the above date-time formats.
 */
static int
dft_dmyhms2tm( char *psz, struct tm *ptm )
{
	size_t idx = 0;

	/* Caller has already skipped blank characters.
	 */
	ptm->tm_wday = 0;

	/* Date formats known here, start with two digits.
	 */
	if ( !isdigit( psz[0] ) || !isdigit( psz[1] ) )
		return 0;

	if ( psz[2] == ':' ) {

		/* HH:MM:SS GMT DD-mth-YY
		 */

		/* hour, min, sec
		 */
		if (
		/*	!isdigit( psz[0] ) || !isdigit( psz[1] ) ||
		 *	psz[2] != ':' ||
		 */
			!isdigit( psz[3] ) || !isdigit( psz[4] ) ||
			psz[5] != ':' ||
			!isdigit( psz[6] ) || !isdigit( psz[7] )
			)
			return 0;

		ptm->tm_hour = (psz[0] - '0') * 10 + (psz[1] - '0');
		ptm->tm_min  = (psz[3] - '0') * 10 + (psz[4] - '0');
		ptm->tm_sec  = (psz[6] - '0') * 10 + (psz[7] - '0');

		idx += 8;
		if ( psz[idx] != ' ')
			return 0;
		do {
			++idx;
		}
		while( psz[idx] == ' ' );

		psz += idx;
		idx = 0;

		if ( psz[0] != 'G' ||
		     psz[1] != 'M' ||
		     psz[2] != 'T' ||
		     psz[3] != ' ' )
			return 0;

		idx += 3;
		do {
			++idx;
		}
		while( psz[idx] == ' ' );
		psz += idx;

		/* day
		 */
		ptm->tm_mday = 0;
		for ( idx = 0; idx < 2 && isdigit( psz[idx] ); ++idx ) {
			ptm->tm_mday = ptm->tm_mday * 10 + (psz[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( psz[idx] != '-')
			return 0;

		++idx;
		psz += idx;

		/* month
		 */
		ptm->tm_mon = 0;
		for ( idx = 0; isalpha( psz[idx] ); ++idx )
			;

		if (! cvt_mon_name2idx( psz, idx, ptm ) )
			return 0;

		if ( psz[idx] != '-')
			return 0;

		++idx;
		psz += idx;

		/* year
		 */
		ptm->tm_year = 0;
		for ( idx = 0; idx < 4 && isdigit( psz[idx] ); ++idx ) {
			ptm->tm_year = ptm->tm_year * 10 + (psz[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( isdigit( psz[idx] ) )
			return 0;

		/* OK, date deformatted and converted.
		 */
		return 1;
	}

	if ( psz[2] == '-') {

		/* DD-mth-YY HH:MM:SS GMT
		 */

		/* day
		 */
		ptm->tm_mday = (psz[0] - '0') * 10 + (psz[1] - '0');
		psz += 3;

		/* month
		*/
		ptm->tm_mon = 0;
		for ( idx = 0; isalpha( psz[idx] ); ++idx )
			;
		if (! cvt_mon_name2idx( psz, idx, ptm ) )
			return 0;

		if ( psz[idx] != '-')
			return 0;
		++idx;
		psz += idx;

		/* year
		 */
		ptm->tm_year = 0;
		for ( idx = 0; idx < 4 && isdigit( psz[idx] ); ++idx ) {
			ptm->tm_year = ptm->tm_year * 10 + (psz[idx] - '0');
		}
		if ( idx == 0 )
			return 0;

		if ( psz[idx] != ' ' )
			return 0;

		do {
			++idx;
		}
		while( psz[idx] == ' ' );

		psz += idx;
		idx = 0;

		/* hour, min, sec
		 */
		if (
			!isdigit( psz[0] ) || !isdigit( psz[1] ) ||
			psz[2] != ':' ||
			!isdigit( psz[3] ) || !isdigit( psz[4] ) ||
			psz[5] != ':' ||
			!isdigit( psz[6] ) || !isdigit( psz[7] )
			)
			return 0;

		ptm->tm_hour = (psz[0] - '0') * 10 + (psz[1] - '0');
		ptm->tm_min  = (psz[3] - '0') * 10 + (psz[4] - '0');
		ptm->tm_sec  = (psz[6] - '0') * 10 + (psz[7] - '0');

		idx += 8;
		if ( psz[idx] != ' ')
			return 0;
		do {
			++idx;
		}
		while( psz[idx] == ' ' );
		psz += idx;
		idx = 0;

		if ( psz[0] != 'G' ||
		     psz[1] != 'M' ||
		     psz[2] != 'T' )
			return 0;
		idx += 3;

		/* OK, date deformatted and converted.
		 */
		return 1;
	}

	/* Unknown date format.
	 */
	return 0;
}


/* It's almost the same as mktime(),
 * excepted for the following assumptions:
 *    - it assumes to handle only UTC/GMT times,
 *      thus ignoring time zone and daylight saving time;
 *    - field values must be right (within the expected ranges).
 */
static time_t
cvt_tm2time( struct tm *ptm )
{
	time_t t;
	int tm_year = ptm->tm_year + 1900;

	/* Years since epoch, converted to days. */
	t = ( ptm->tm_year - 70 ) * 365;

	/* Leap days for previous years. */
	t += ( ptm->tm_year - 69 ) / 4;

	/* Days for the beginning of this month. */
	t += month_ydays_tab[ptm->tm_mon];

	/* Leap day for this year. */
	if ( ptm->tm_mon >= 2 && is_leap( tm_year ) )
		++t;

	/* Days since the beginning of this month. */
	t += ptm->tm_mday - 1; /* 1-based field */

	/* Hours, minutes, and seconds. */
	t = t * 24 + ptm->tm_hour;
	t = t * 60 + ptm->tm_min;
	t = t * 60 + ptm->tm_sec;

	return t;
}


/*
 * Parses a date time string from one of the http formats
 * (commonly used) to a time_t time.
 * 'cstr' is assumed to be a zero terminated string.
 *
 * Returned values:
 *   ret_ok    - Parsed okay
 *   ret_error - Invalid date string
 *   ret_deny  - Parsed an invalid date
 */
ret_t
cherokee_dtm_str2time (char* cstr, int cstr_len, time_t *time)
{
	struct tm   tm;
	char       *psz      = cstr;
	size_t      idx      = 0;
	const char *cstr_end = cstr + cstr_len;

	/* Zero struct tm.
	 */
	(void) memset( (char*) &tm, 0, sizeof(struct tm) );

	/* Skip initial blank character(s).
	 */
	while ( *psz == ' ' || *psz == '\t' )
		++psz;

	/* Guess category of date time.
	 */
	if ( isalpha( *psz ) ) {
		/* wdy[,] ...
		 * wdy = short or long name of the day of week.
		 */

		/* deformat day (name) of week
		 */
		for ( idx = 0; isalpha( psz[idx] ); ++idx )
			;
		if (! cvt_wday_name2idx( psz, idx, &tm ) )
			return ret_error;

		/* Another guess of the type of date time format.
		 */
		if ( psz[idx] == ',' ) {

			/* -----------------------------
			 * wdy, DD mth YYYY HH:MM:SS GMT
			 * wdy, DD mth YYYY HH:MM:SS +0000
			 * wdy, DD-mth-YY HH:MM:SS GMT
			 * -----------------------------
			 */

			/* Skip white spaces.
			 */
			++idx;
			if ( psz[idx] != ' ')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' );
			psz += idx;

			/* Deformat day of month.
			 */
			for ( idx = 0; idx < 2 && isdigit( psz[idx] ); ++idx ) {
				tm.tm_mday = tm.tm_mday * 10 + (psz[idx] - '0');
			}
			if ( idx == 0 )
				return ret_error;

			/* Skip field separator(s).
			 */
			if ( psz[idx] != ' ' && psz[idx] != '-')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' || psz[idx] == '-' );
			psz += idx;

			/* Deformat month.
			 */
			for ( idx = 0; isalpha( psz[idx] ); ++idx )
				;
			if (! cvt_mon_name2idx( psz, idx, &tm ) )
				return ret_error;

			/* Skip field separator(s).
			 */
			if ( psz[idx] != ' ' && psz[idx] != '-')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' || psz[idx] == '-' );
			psz += idx;

			/* Deformat year.
			 */
			for ( idx = 0; idx < 4 && isdigit( psz[idx] ); ++idx ) {
				tm.tm_year = tm.tm_year * 10 + (psz[idx] - '0');
			}
			if ( idx == 0 )
				return ret_error;

			/* Skip field separator(s).
			 */
			if ( psz[idx] != ' ')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' );
			psz += idx;
			idx = 0;

			/* Deformat hours, minutes, seconds.
			 */
			if ((cstr_end - psz < 7)||
			    (! isdigit (psz[0]) || ! isdigit (psz[1]) || psz[2] != ':' ||
			     ! isdigit (psz[3]) || ! isdigit (psz[4]) || psz[5] != ':' ||
			     ! isdigit (psz[6]) || ! isdigit (psz[7])))
			{
				return ret_error;
			}

			tm.tm_hour = (psz[0] - '0') * 10 + (psz[1] - '0');
			tm.tm_min  = (psz[3] - '0') * 10 + (psz[4] - '0');
			tm.tm_sec  = (psz[6] - '0') * 10 + (psz[7] - '0');

			/* Skip field separator(s).
			 */
			idx += 8;
			while (psz[idx] == ' ')
				++idx;
			psz += idx;
			idx = 0;

			/* Time Zone (always Greenwitch Mean Time)
			 */
			if (cstr_end - psz < 3) {
				return ret_error;
			}

			if ((psz[0] != 'G' || psz[1] != 'M' || psz[2] != 'T') &&
			    (psz[0] != '+' || psz[1] != '0' || psz[2] != '0'))
			{
				return ret_error;
			}

		} else {

			/* --------------------------
			 * wdy mth DD HH:MM:SS YYYY
			 * wdy mth DD HH:MM:SS GMT YY
			 * --------------------------
			 */

			if ( psz[idx] != ' ')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' );
			psz += idx;

			/* Deformat month.
			 */
			for ( idx = 0; isalpha( psz[idx] ); ++idx )
				;
			if (! cvt_mon_name2idx( psz, idx, &tm ) )
				return ret_error;

			/* Skip field separator(s).
			 */
			if ( psz[idx] != ' ')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ');
			psz += idx;

			/* Deformat day of month.
			 */
			for ( idx = 0; idx < 2 && isdigit( psz[idx] ); ++idx ) {
				tm.tm_mday = tm.tm_mday * 10 + (psz[idx] - '0');
			}
			if ( idx == 0 )
				return ret_error;

			/* Skip field separator(s).
			 */
			if ( psz[idx] != ' ')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' );
			psz += idx;
			idx = 0;

			/* Deformat hours, minutes, seconds.
			 */
			if ((cstr_end - psz < 7)||
			    (! isdigit (psz[0]) || ! isdigit (psz[1]) || psz[2] != ':' ||
			     ! isdigit (psz[3]) || ! isdigit (psz[4]) || psz[5] != ':' ||
			     ! isdigit (psz[6]) || ! isdigit (psz[7])))
			{
				return ret_error;
			}

			tm.tm_hour = (psz[0] - '0') * 10 + (psz[1] - '0');
			tm.tm_min  = (psz[3] - '0') * 10 + (psz[4] - '0');
			tm.tm_sec  = (psz[6] - '0') * 10 + (psz[7] - '0');

			/* Skip field separator(s).
			 */
			idx += 8;
			if ( psz[idx] != ' ')
				return ret_error;
			do {
				++idx;
			}
			while( psz[idx] == ' ' );
			psz += idx;
			idx = 0;

			/* Optional Time Zone (always Greenwitch Mean Time)
			 */
			if ( psz[0] == 'G' ) {
				if ( psz[1] != 'M' ||
				     psz[2] != 'T' ||
				     psz[3] != ' ' )
					return ret_error;
				idx = 3;
				do {
					++idx;
				}
				while ( psz[idx] == ' ' );
				psz += idx;
				idx = 0;
			}
			/* else C asctime() format
			 */

			/* Deformat year.
			 */
			for ( idx = 0; idx < 4 && isdigit( psz[idx] ); ++idx ) {
				tm.tm_year = tm.tm_year * 10 + (psz[idx] - '0');
			}
			if ( idx == 0 )
				return ret_error;

			if ( isdigit( psz[idx] ) )
				return ret_error;
			psz += idx;
			idx = 0;

		}
	} else
	if ( isdigit( *psz ) ) {
		/* Uncommon date-time formats
		 * --------------------------
		 * HH:MM:SS GMT DD-mth-YY
		 * DD-mth-YY HH:MM:SS GMT
		 * --------------------------
		 */
		if ( !dft_dmyhms2tm( psz, &tm ) )
			return ret_error;

	} else {
		/* Bad date or unknown date-time format
		 */
		return ret_error;
	}

	if ( tm.tm_year >  1900 )
		tm.tm_year -= 1900;
	else
	if ( tm.tm_year < 70 )
		tm.tm_year += 100;

	/* Test field values
	 * NOTE: time has to be in the range 01-Jan-1970 - 31-Dec-2036.
	 */
	if ( tm.tm_year < 70 || tm.tm_year > 136 ||
	  /* it's guaranteed that tm_mon is always within this range:
	   * tm.tm_mon  <  0 || tm.tm_mon  >  11 ||
	   */
	     tm.tm_mday <  1 || tm.tm_mday >  31 ||
	     tm.tm_hour <  0 || tm.tm_hour >  23 ||
	     tm.tm_min  <  0 || tm.tm_min  >  59 ||
	     tm.tm_sec  <  0 || tm.tm_sec  >  59 )
		return ret_deny;

	/* OK, convert struct tm to time_t and return result.
	 */
	*time = cvt_tm2time (&tm);
	return ret_ok;
}


/* Format an RFC1123 GMT time assuming that (ptm) parameter
 * refers to GMT timezone without daylight savings (of course).
 * NOTE: in HTTP headers, week day and month names MUST be in English !
 */
size_t
cherokee_dtm_gmttm2str( char *bufstr, size_t bufsize, struct tm *ptm )
{
	unsigned int uYear;

	if ( ptm == NULL || bufsize < DTM_SIZE_GMTTM_STR ) {
		bufstr[0] = '\0';
		return 0;
	}

	uYear = (unsigned int) ptm->tm_year + 1900;

	bufstr[ 0] = wday_name_tab[ ptm->tm_wday ].name1[0];
	bufstr[ 1] = wday_name_tab[ ptm->tm_wday ].name1[1];
	bufstr[ 2] = wday_name_tab[ ptm->tm_wday ].name1[2];
	bufstr[ 3] = ',';
	bufstr[ 4] = ' ';
	bufstr[ 5] = (char) ('0' + (ptm->tm_mday / 10) );
	bufstr[ 6] = (char) ('0' + (ptm->tm_mday % 10) );
	bufstr[ 7] = ' ';
	bufstr[ 8] = month_name_tab[ ptm->tm_mon ].name1[0];
	bufstr[ 9] = month_name_tab[ ptm->tm_mon ].name1[1];
	bufstr[10] = month_name_tab[ ptm->tm_mon ].name1[2];
	bufstr[11] = ' ';
	bufstr[12] = (char) ( '0' + ( uYear / 1000 ) % 10 );
	bufstr[13] = (char) ( '0' + ( uYear /  100 ) % 10 );
	bufstr[14] = (char) ( '0' + ( uYear /   10 ) % 10 );
	bufstr[15] = (char) ( '0' + ( uYear %   10 ) );
	bufstr[16] = ' ';
	bufstr[17] = (char) ( '0' + ( ptm->tm_hour / 10 ) );
	bufstr[18] = (char) ( '0' + ( ptm->tm_hour % 10 ) );
	bufstr[19] = ':';
	bufstr[20] = (char) ( '0' + ( ptm->tm_min  / 10 ) );
	bufstr[21] = (char) ( '0' + ( ptm->tm_min  % 10 ) );
	bufstr[22] = ':';
	bufstr[23] = (char) ( '0' + ( ptm->tm_sec  / 10 ) );
	bufstr[24] = (char) ( '0' + ( ptm->tm_sec  % 10 ) );
	bufstr[25] = ' ';
	bufstr[26] = 'G';
	bufstr[27] = 'M';
	bufstr[28] = 'T';
	bufstr[29] = '\0';

#if    DTM_LEN_GMTTM_STR != 29
#error DTM_LEN_GMTTM_STR != 29
#endif /* DTM_LEN_GMTTM_STR */

	/* Return constant length
	 */
	return DTM_LEN_GMTTM_STR;
}

/* EOF */
