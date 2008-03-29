/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
#include "cacheline.h"


#if (( __GNUC__ || __INTEL_COMPILER ) && ( __i386__ || __amd64__ ))

static void
get_cpuid (uint32_t i, uint32_t *buf)
{
# if __i386__
	__asm__ (

		"    mov    %%ebx, %%esi;  "

		"    cpuid;                "
		"    mov    %%eax, (%1);   "
		"    mov    %%ebx, 4(%1);  "
		"    mov    %%edx, 8(%1);  "
		"    mov    %%ecx, 12(%1); "

		"    mov    %%esi, %%ebx;  "

		: : "a" (i), "D" (buf) : "ecx", "edx", "esi", "memory" );

# elif __amd64__

	uint32_t  eax, ebx, ecx, edx;

	__asm__ (
		"cpuid"
		: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (i) );

	buf[0] = eax;
	buf[1] = ebx;
	buf[2] = edx;
	buf[3] = ecx;

# endif
}

static ret_t 
get_cache_line_live (cuint_t *size) 
{
	uint32_t    vendor_buf[5];
	uint32_t processor_buf[5];
	
	const char *vendor;

	memset (vendor_buf, 0, sizeof(vendor_buf));
	memset (processor_buf, 0, sizeof(processor_buf));

	/* EAX=0: Get vendor ID 
	 */
	get_cpuid (0, vendor_buf);
	vendor = (const char *)&vendor_buf[1];

	if (vendor_buf[0] == 0) {
		return ret_error;
	}
	
	/* EAX=1: Get Processor Info and Feature Bits 
	 */
	get_cpuid(1, processor_buf);
	if (strcmp (vendor, "GenuineIntel") == 0) {
		switch ((processor_buf[0] & 0xf00) >> 8) {
		case 5:
			/* Pentium */
			*size = 32;
			return ret_ok;

		case 6:
			/* Pentium Pro, II, III */
			*size = 32;

			if ((processor_buf[0] & 0xf0) >= 0xd0) {
				/* Intel Core */
				*size = 64;
			}
			return ret_ok;

		case 15:
			/* Pentium 4, although its cache line size is 64 bytes,
			 * it prefetches up to two cache lines during memory read
			 */
			*size = 128;
			return ret_ok;
		}
		
	} else if (strcmp(vendor, "AuthenticAMD") == 0) {
		*size = 64;
		return ret_ok;
	}

	return ret_not_found;
}

#endif  // (( __GNUC__ || __INTEL_COMPILER ) && ( __i386__ || __amd64__ ))


ret_t
cherokee_cacheline_size_get (cuint_t *size) 
{
#if (( __GNUC__ || __INTEL_COMPILER ) && ( __i386__ || __amd64__ ))
	ret_t ret;
	ret = get_cache_line_live (size);
	if (ret == ret_ok) return ret_ok;
#endif

	*size = CPU_CACHE_LINE;
	return ret_ok;
}
