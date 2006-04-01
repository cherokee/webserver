/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee replacement utility
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif


static char *
cherokee_strcasestr (char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while (sc != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *) s);
}


static char* 
str_replace (const char* str, const char* needle1, const char* needle2)
{
	char *var;
	char *tmp_pos;
	char *needle_pos;
	int   count;
	int   len;
		
	/* Calculate the size of the new string
	 */
	len = strlen(str);

	if (strlen (needle1) < strlen (needle2)) {
		count   = 0;
		tmp_pos = (char*)str;
			
		while ((needle_pos = (char*) cherokee_strcasestr(tmp_pos, needle1))) {
			tmp_pos = needle_pos + strlen (needle1);
			count++;
		}

		len += strlen(str) + (strlen(needle2) - strlen(needle1)) * count;
	}
	   
	/* Get memory for the new string
	 */
	var = (char*) malloc (sizeof(char) * (len+1));
	memset (var, 0, sizeof(char) * (len+1));

	/* New string building
	 */
	tmp_pos = (char*) str;
	while ((needle_pos = (char *) cherokee_strcasestr(tmp_pos, needle1))) {
		len = needle_pos - tmp_pos;
		strncat (var, tmp_pos, len);
		strcat (var, needle2);
		tmp_pos = needle_pos + strlen (needle1);
	}

	strcat( var, tmp_pos );
		
	return var;
}


int
main (int argc, char *argv[]) 
{
	int          re,i;
	FILE        *file;
	char        *filename_in;
	char        *filename_out;
	char        *content;
	char        *content_new;
	struct stat  info;

	/* Check the arguments
	 */
	if (argc < 5) {
		fprintf (stderr, "Usage:\n%s infile outfile replace_str subject_str\n\n", argv[0]);
		exit(1);
	}

	filename_in  = argv[1];
	filename_out = argv[2];

	/* Read the file content
	 */
	re = stat (filename_in, &info);
	if (re < 0) {
		fprintf (stderr, "%s: File not found: %s\n", argv[0], filename_in);
		exit(2);
	}

	file = fopen (filename_in, "r");
	if (file == NULL) {
		fprintf (stderr, "%s: Can not open %s", argv[0], filename_in);
		exit(3);
	}
	   
	content = (char *) malloc (sizeof(char) * (info.st_size+1));
	fread (content, 1, info.st_size, file);
	fclose (file);

	/* Replace the strings
	 */
	for (i=0; i< (argc-3)/2; i++) {
		char *needle1 = argv[(i*2)+3];
		char *needle2 = argv[(i*2)+4];

		content_new = str_replace (content, needle1, needle2);

		free (content);
		content = content_new;
	}

	/* Write down the new file
	 */
	file = (strcmp (filename_out, "-") == 0) ? stdout : fopen (filename_out, "w");
	fwrite (content, 1, strlen(content), file);
	fclose (file);

	free (content);

	return 0;
}

