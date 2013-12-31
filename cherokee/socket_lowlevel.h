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

#ifndef CHEROKEE_SOCKET_LOWLEVEL_H
#define CHEROKEE_SOCKET_LOWLEVEL_H

#include "common-internal.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <sys/types.h>
#include <netdb.h>

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
# include <sys/un.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif



#ifdef INET6_ADDRSTRLEN
# define CHE_INET_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#  ifdef INET_ADDRSTRLEN
#    define CHE_INET_ADDRSTRLEN INET_ADDRSTRLEN
#  else
#    define CHE_INET_ADDRSTRLEN 16
#  endif
#endif

#ifndef AF_LOCAL
# define AF_LOCAL AF_UNIX
#endif

#ifndef SUN_LEN
#define SUN_LEN(sa)                                    \
	(strlen((sa)->sun_path) +                      \
	 (size_t)(((struct sockaddr_un*)0)->sun_path))
#endif

#ifndef SUN_ABSTRACT_LEN
#define SUN_ABSTRACT_LEN(sa)                           \
	(strlen((sa)->sun_path+1) + 2 +                \
	 (size_t)(((struct sockaddr_un*)0)->sun_path))
#endif


typedef struct {
	union {
		struct in_addr  addr_ipv4;
		struct in6_addr addr_ipv6;
	} addr;
	unsigned short family;
} cherokee_in_addr_t;


#endif /* CHEROKEE_SOCKET_LOWLEVEL_H */
