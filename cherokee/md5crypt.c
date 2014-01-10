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
#include "md5.h"
#include "md5crypt.h"
#include "util.h"

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some
 * day, and you think this stuff is worth it, you can buy me a beer in
 * return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

/* 0 ... 63 => ascii - 64 */
static unsigned char itoa64[] =
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static char *
to64 (char *tmp, unsigned long v, int n)
{
	/* We do know 'tmp' is 5 bytes long */
	char *s = tmp;
	memset (tmp, 0, 5);

	if (n > 4)
		return NULL;

	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}

	return tmp;
}


char *
md5_crypt(const char *pw, const char *salt, const char *magic, char passwd[MD5CRYPT_PASSWD_LEN])
{
	char salt_copy[9];
	const char *sp, *ep;
	unsigned char final[16];
	int sl, pl, i, j;
	struct MD5Context ctx, ctx1;
	unsigned long l;
	cuint_t magic_len;
	char  to64_buf[5];

	/* Refine the salt first.  It's possible we were given an already-hashed
	 * string as the salt argument, so extract the actual salt value from it
	 * if so.  Otherwise just use the string up to the first '$' as the salt.
	 */
	sp = salt;

	/* If it starts with the magic string, then skip that.
	 */
	magic_len = strlen (magic);

	if (strncmp(sp, magic, magic_len) == 0)
		sp += magic_len;

	/* It stops at the first '$', max 8 chars
	 */
	for (ep = sp; *ep != '$'; ep++) {
		if (*ep == '\0' || ep >= (sp + 8))
			return NULL;
	}

	/* Get the length of the true salt
	 */
	sl = ep - sp;

	/* Stash the salt */
	memcpy(salt_copy, sp, sl);
	salt_copy[sl] = '\0';

	MD5Init(&ctx);

	/* The password first, since that is what is most unknown */
	MD5Update(&ctx, (unsigned char *)pw, strlen(pw));

	/* Then our magic string */
	MD5Update(&ctx, (unsigned char *)magic, magic_len);

	/* Then the raw salt */
	MD5Update(&ctx, (unsigned char *)sp, sl);

	/* Then just as many characters of the MD5(pw, salt, pw) */
	MD5Init(&ctx1);
	MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
	MD5Update(&ctx1, (unsigned char *)sp, sl);
	MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
	MD5Final(final, &ctx1);

	for(pl = strlen(pw); pl > 0; pl -= 16)
		MD5Update(&ctx, final, pl > 16 ? 16 : pl);

	/* Don't leave anything around in vm they could use. */
	memset(final, '\0', sizeof final);

	/* Then something really weird... */
	for (j = 0, i = strlen(pw); i != 0; i >>= 1) {
		if (i & 1)
			MD5Update(&ctx, final + j, 1);
		else
			MD5Update(&ctx, (unsigned char *)pw + j, 1);
	}

	/* Now make the output string
	 */

	snprintf(passwd, MD5CRYPT_PASSWD_LEN, "%s%s$", magic, salt_copy);

	MD5Final(final, &ctx);

	/*
	 * and now, just to make sure things don't run too fast
	 * On a 60 Mhz Pentium this takes 34 msec, so you would
	 * need 30 seconds to build a 1000 entry dictionary...
	 */
	for(i = 0; i < 1000; i++) {
		MD5Init(&ctx1);
		if (i & 1)
			MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
		else
			MD5Update(&ctx1, final, 16);

		if (i % 3)
			MD5Update(&ctx1, (unsigned char *)sp, sl);

		if (i % 7)
			MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));

		if (i & 1)
			MD5Update(&ctx1, final, 16);
		else
			MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));

		MD5Final(final, &ctx1);
	}

	l = (final[ 0]<<16) | (final[ 6]<<8) | final[12];
	strlcat (passwd, to64(to64_buf, l, 4), MD5CRYPT_PASSWD_LEN);
	l = (final[ 1]<<16) | (final[ 7]<<8) | final[13];
	strlcat (passwd, to64(to64_buf, l, 4), MD5CRYPT_PASSWD_LEN);
	l = (final[ 2]<<16) | (final[ 8]<<8) | final[14];
	strlcat (passwd, to64(to64_buf, l, 4), MD5CRYPT_PASSWD_LEN);
	l = (final[ 3]<<16) | (final[ 9]<<8) | final[15];
	strlcat (passwd, to64(to64_buf, l, 4), MD5CRYPT_PASSWD_LEN);
	l = (final[ 4]<<16) | (final[10]<<8) | final[ 5];
	strlcat (passwd, to64(to64_buf, l, 4), MD5CRYPT_PASSWD_LEN);
	l =                    final[11]                ;
	strlcat (passwd, to64(to64_buf, l, 2), MD5CRYPT_PASSWD_LEN);

	/* Don't leave anything around in vm they could use.
	 */
	memset(final, 0, sizeof(final));
	memset(salt_copy, 0, sizeof(salt_copy));
	memset(&ctx, 0, sizeof(ctx));
	memset(&ctx1, 0, sizeof(ctx1));
	(void) to64(to64_buf, 0, 4);

	return passwd;
}
