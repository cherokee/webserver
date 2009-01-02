/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

/* $Id: md5crypt.h,v 1.4 2003/05/18 14:46:46 djm Exp $ */

#ifndef CHEROKEE_MD5CRYPT_H
#define CHEROKEE_MD5CRYPT_H

#include "common.h"

#define MD5CRYPT_PASSWD_LEN 120

char *md5_crypt (const char *pass, const char *salt, const char *magic, char passwd[MD5CRYPT_PASSWD_LEN]);

#endif /* CHEROKEE_MD5CRYPT_H */

