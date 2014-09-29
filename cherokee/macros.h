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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif


#ifndef CHEROKEE_MACROS_H
#define CHEROKEE_MACROS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef  __cplusplus
#  define CHEROKEE_BEGIN_DECLS  extern "C" {
#  define CHEROKEE_END_DECLS    }
#else
#  define CHEROKEE_BEGIN_DECLS
#  define CHEROKEE_END_DECLS
#endif

#define __cherokee_func__ __func__

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#ifndef NULL
#  ifdef __cplusplus
#    define NULL        (0L)
#  else /* !__cplusplus */
#    define NULL        ((void*) 0)
#  endif /* !__cplusplus */
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
# define likely(x)   __builtin_expect((x),1)
# define unlikely(x) __builtin_expect((x),0)
# define not_ok(x)   __builtin_expect(x != ret_ok, 0)
# define lt_ok(x)    __builtin_expect(x <  ret_ok, 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
# define not_ok(x)   (x != ret_ok)
# define lt_ok(x)    (x <  ret_ok)
#endif

#if defined(__GNUC__)
# define __GNUC_VERSION \
	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
# else
# define __GNUC_VERSION 0
#endif

#if __GNUC_VERSION >= 30000
# define must_check  __attribute__ ((warn_unused_result))
# define no_return   __attribute__ ((noreturn))
#else
# define must_check
# define no_return
#endif

#define DEFAULT_RECV_SIZE             ( 2 * 1024)
#define DEFAULT_READ_SIZE             (16 * 1024)
#define MAX_HEADER_LEN                8192
#define MAX_HEADER_CRLF               8
#define MAX_KEEPALIVE                 500
#define MAX_NEW_CONNECTIONS_PER_STEP  50
#define DEFAULT_CONN_REUSE            20
#define TERMINAL_WIDTH                80
#define DEFAULT_TRAFFIC_UPDATE        10
#define CGI_TIMEOUT                   65
#define SECONDS_TO_LINGER             2
#define LOGGER_MIN_BUFSIZE            0
#define DEFAULT_LOGGER_MAX_BUFSIZE    32768
#define LOGGER_MAX_BUFSIZE            (4 * 1024 * 1024)
#define LOGGER_FLUSH_LAPSE            10
#define RESPINS_MAX                   16
#define SENDFILE_MIN_SIZE             (128 * 1024)   /* 128Kb */
#define SENDFILE_MAX_SIZE             2147483647     /* 2Gb -1*/
#define NONCE_CLEANUP_LAPSE           60
#define NONCE_EXPIRATION              60
#define POST_READ_SIZE                32700
#define FLCACHE_LAPSE                 60

#define FD_NUM_SPARE                  10        /* range:  8 ... 20    */
#define FD_NUM_MIN_SYSTEM             20        /* range: 16 ... 64    */
#define FD_NUM_MIN_AVAILABLE          8         /* range:  8 ... 65000 */
#define FD_NUM_MIN_PER_THREAD         8         /* range:  8 ... 65000 */
#define FD_NUM_CUSTOM_LIMIT           4096      /* range: 16 ... 65000 */

#define EXIT_OK                       0
#define EXIT_ERROR                    1
#define EXIT_OK_ONCE                  2
#define EXIT_ERROR_FATAL              3

#if (FD_NUM_MIN_SYSTEM < 16)
# error FD_NUM_MIN_SYSTEM too low, < 16
#endif
#if (FD_NUM_SPARE < 8)
# error FD_NUM_SPARE too low, < 8
#endif
#if (FD_NUM_MIN_AVAILABLE < 8)
# error FD_NUM_MIN_AVAILABLE too low, < 8
#endif
#if (FD_NUM_MIN_PER_THREAD < 8)
# error FD_NUM_MIN_PER_THREAD too low, < 8
#endif
#if (FD_NUM_MIN_PER_THREAD > FD_NUM_MIN_AVAILABLE)
# error FD_NUM_MIN_PER_THREAD too high, > FD_NUM_MIN_AVAILABLE
#endif
#if ((FD_NUM_MIN_SYSTEM - FD_NUM_SPARE) < FD_NUM_MIN_AVAILABLE)
# error FD_NUM_MIN_SYSTEM too low or FD_NUM_SPARE FDS too high
#endif

#define CSZLEN(str) (sizeof(str) - 1)

#define LF_LF     "\n\n"        /* EOHs (End Of Headers) */
#define CRLF_CRLF "\r\n\r\n"    /* EOHs (End Of Headers) */
#define CRLF      "\r\n"        /* EOH (End Of Header Line) */
#define LWS       " \t\r\n"     /* HTTP linear white space */
#define LBS       " \t"         /* HTTP linear blank space */
#define CHR_CR    '\r'          /* 0x0D: Carriage return */
#define CHR_LF    '\n'          /* 0x0A: Line feed (new line) */
#define CHR_SP    ' '           /* Space */
#define CHR_HT    '\t'          /* Horizontal tab */


#define equal_str(m,str) \
	(strncasecmp(m, str, sizeof(str)-1) == 0)

#define equal_buf_str(b,str) \
	(cherokee_buffer_case_cmp_str((b),(str)) == 0)

#define return_if_fail(expr,ret) \
	do {                                                            \
		if (!(expr)) {                                          \
			PRINT_ERROR ("assertion `%s' failed\n", #expr); \
			return (ret);                                   \
		}                                                       \
	} while(0)

#define SHOULDNT_HAPPEN                                                 \
	do {                                                            \
		fprintf (stderr,                                        \
			 "file %s:%d (%s): this should not happen\n",   \
			 __FILE__, __LINE__, __cherokee_func__);        \
		fflush (stderr);                                        \
	} while (0)

#define RET_UNKNOWN(ret)                                                \
	do {                                                            \
		fprintf (stderr,                                        \
			 "file %s:%d (%s): ret code unknown ret=%d\n",  \
			 __FILE__, __LINE__, __cherokee_func__, ret);   \
		fflush (stderr);                                        \
	} while (0)

#define UNUSED(x) ((void)(x))

/* Make cherokee type.
 */
#define CHEROKEE_MK_TYPE(type) \
	cherokee_ ## type ## _t

/* Make cherokee new type function.
 */
#define CHEROKEE_MK_NEW(obj,type) \
	cherokee_ ## type ## _new (& obj )

/* Declare struct.
 */
#define CHEROKEE_DCL_STRUCT(obj,type) \
	CHEROKEE_MK_TYPE(type) * obj

/* Declare and allocate a new struct.
 */
#define CHEROKEE_NEW_STRUCT(obj,type)                              \
	CHEROKEE_DCL_STRUCT(obj,type) = (CHEROKEE_MK_TYPE(type) *) \
	malloc (sizeof(CHEROKEE_MK_TYPE(type)));                   \
	return_if_fail (obj != NULL, ret_nomem)

/* Declare and allocate a zeroed new struct.
 */
#define CHEROKEE_CNEW_STRUCT(nmemb,obj,type)                       \
	CHEROKEE_DCL_STRUCT(obj,type) = (CHEROKEE_MK_TYPE(type) *) \
	calloc ((nmemb), sizeof(CHEROKEE_MK_TYPE(type)));          \
	return_if_fail (obj != NULL, ret_nomem)

/* Declare and initialize a new object.
 */
#define CHEROKEE_NEW(obj,type)                   \
	CHEROKEE_DCL_STRUCT(obj,type) = NULL;    \
	CHEROKEE_MK_NEW(obj,type);               \
	return_if_fail (obj != NULL, ret_nomem)

/* Decleare and initialize two new object.
 * NOTE: if the second object allocation fails,
 *       then we leak memory of obj1, anyway, as this is a half disaster,
 *       and it is likely that cherokee will exit because of this,
 *       we don't care too much (for now).
 */
#define CHEROKEE_NEW2(obj1,obj2,type)             \
	CHEROKEE_DCL_STRUCT(obj1,type) = NULL;    \
	CHEROKEE_DCL_STRUCT(obj2,type) = NULL;    \
	CHEROKEE_MK_NEW(obj1,type);               \
	CHEROKEE_MK_NEW(obj2,type);               \
	return_if_fail (obj1 != NULL, ret_nomem)  \
	return_if_fail (obj2 != NULL, ret_nomem)

#define CHEROKEE_NEW_TYPE(obj,type) \
	type * obj = (type *) malloc(sizeof(type)); \
	return_if_fail (obj != NULL, ret_nomem)

/* We assume to have a C ANSI free().
 */
#define CHEROKEE_FREE(obj)    \
	do {                  \
		free (obj);   \
		(obj) = NULL; \
	} while (0)

#define CHEROKEE_TEMP(obj, size)                 \
    const unsigned int obj ## _size = size;  \
char obj[size]

#ifndef MIN
# define MIN(x,y) ((x<y) ? x : y)
#endif

#ifndef MAX
# define MAX(x,y) ((x>y) ? x : y)
#endif

/* Automatic functions:
 * These macros implement _new/_free by using _init/_mrproper.
 */
#define CHEROKEE_ADD_FUNC_NEW(klass)  \
ret_t                                                         \
cherokee_ ## klass ## _new (cherokee_ ## klass ## _t **obj) { \
	ret_t ret;                                            \
	CHEROKEE_NEW_STRUCT (n, klass);                       \
	                                                      \
	ret = cherokee_ ## klass ## _init (n);                \
	if (unlikely (ret != ret_ok)) {                       \
		free (n);                                     \
		return ret;                                   \
	}                                                     \
	                                                      \
	*obj = n;                                             \
	return ret_ok;                                        \
}

#define CHEROKEE_ADD_FUNC_FREE(klass)  \
    ret_t                                                         \
cherokee_ ## klass ## _free (cherokee_ ## klass ## _t *obj) { \
    if ((obj) == NULL)                                    \
    return ret_ok;                                \
    \
    cherokee_ ## klass ## _mrproper (obj);                \
    \
    free (obj);                                           \
    return ret_ok;                                        \
}


/* Printing macros
*/
#if defined(__GNUC__) || ( defined(__SUNPRO_C) && __SUNPRO_C > 0x590 )
# define PRINT_MSG(fmt,arg...)    do { fprintf(stderr, fmt, ##arg); fflush(stderr); } while(0)
# define PRINT_ERROR(fmt,arg...)  do { fprintf(stderr, "%s:%d - "fmt, __FILE__, __LINE__, ##arg); fflush(stderr); } while(0)
#else
# define PRINT_MSG(fmt,...)       do { fprintf(stderr, fmt, __VA_ARGS__); fflush(stderr); } while(0)
# define PRINT_ERROR(fmt,...)     do { fprintf(stderr, "%s:%d - "fmt, __FILE__, __LINE__, __VA_ARGS__); fflush(stderr); } while(0)
#endif

#ifdef DEBUG
# ifdef __GNUC__
#  define PRINT_DEBUG(fmt,arg...) do { fprintf(stdout, "%s:%d - " fmt,__FILE__,__LINE__,##arg); fflush(stdout); } while (0)
# else
#  define PRINT_DEBUG(fmt,...)    do { fprintf(stdout, "%s:%d - " fmt,__FILE__,__LINE__,__VA_ARGS__); fflush(stdout); } while (0)
# endif
#else
# ifdef __GNUC__
#  define PRINT_DEBUG(fmt,arg...) do { } while(0)
# else
#  define PRINT_DEBUG(fmt,...)    do { } while(0)
# endif
#endif

#define PRINT_ERROR_S(str)        PRINT_ERROR("%s",str)
#define PRINT_MSG_S(str)          PRINT_MSG("%s",str)

/* Tracing facility
*/
#define TRACE_ENV "CHEROKEE_TRACE"

#ifdef TRACE_ENABLED
# ifdef __GNUC__
#  define TRACE(fmt,arg...) cherokee_trace_do_trace (fmt, __FILE__, __LINE__, __cherokee_func__, ##arg)
# else
#  define TRACE(fmt,...) cherokee_trace_do_trace (fmt, __FILE__, __LINE__, __cherokee_func__, __VA_ARGS__)
# endif
#else
# ifdef __GNUC__
#  define TRACE(section,fmt,arg...) do { } while(0)
# else
#  define TRACE(section,fmt,...) do { } while(0)
# endif
#endif

/* Conversions
*/
#define POINTER_TO_INT(pointer) ((long)(pointer))
#define INT_TO_POINTER(integer) ((void*)((long)(integer)))

/* Bit masks
*/
#define BIT_SET(var,bit)    var |= (bit)
#define BIT_UNSET(var,bit)  var &= ~(bit)

/* Format string for off_t and size_t
*/
#if (SIZEOF_OFF_T == SIZEOF_UNSIGNED_LONG_LONG)
# define FMT_OFFSET "%llu"
# define FMT_OFFSET_HEX "%llx"
# define CST_OFFSET unsigned long long
#elif (SIZEOF_OFF_T == SIZEOF_UNSIGNED_LONG)
# define FMT_OFFSET "%lu"
# define FMT_OFFSET_HEX "%lx"
# define CST_OFFSET unsigned long
#else
# error Unknown size of off_t
#endif

#if (SIZEOF_SIZE_T == SIZEOF_UNSIGNED_INT)
# define FMT_SIZE "%d"
# define FMT_SIZE_HEX "%x"
# define CST_SIZE unsigned int
#elif (SIZEOF_SIZE_T == SIZEOF_UNSIGNED_LONG_LONG)
# define FMT_SIZE "%llu"
# define FMT_SIZE_HEX "%llx"
# define CST_SIZE unsigned long long
#else
# error Unknown size of size_t
#endif


#ifndef va_copy
# ifdef __va_copy
#  define va_copy(DEST,SRC) __va_copy((DEST),(SRC))
# else
#  ifdef HAVE_VA_LIST_AS_ARRAY
#   define va_copy(DEST,SRC) (*(DEST) = *(SRC))
#  else
#   define va_copy(DEST,SRC) ((DEST) = (SRC))
#  endif
# endif
#endif


# define CHEROKEE_EXPORT

# define SLASH '/'

#define CHEROKEE_CRASH             \
	do {                       \
		*((int *)(0)) = 1; \
	} while(0)

#define CHEROKEE_PRINT_BACKTRACE                                \
	do {                                                    \
		cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;      \
		cherokee_buf_add_backtrace (&tmp, 0, "\n", ""); \
		PRINT_MSG ("%s", tmp.buf);                      \
		cherokee_buffer_mrproper (&tmp);                \
	} while (0)


#define CHEROKEE_CHAR_IS_WHITE(_ch)     (((_ch) == ' ') || ((_ch) == '\t') || ((_ch) == '\n'))
#define CHEROKEE_CHAR_IS_UPPERCASE(_ch) (((_ch) >= 'A') && ((_ch) <= 'Z'))
#define CHEROKEE_CHAR_IS_LOWERCASE(_ch) (((_ch) >= 'a') && ((_ch) <= 'z'))
#define CHEROKEE_CHAR_IS_DIGIT(_ch)     (((_ch) >= '0') && ((_ch) <= '9'))
#define CHEROKEE_CHAR_TO_LOWER(_ch)     ((_ch) | 32)
#define CHEROKEE_CHAT_TO_UPPER(_ch)     ((_ch) & ~32)


#endif /* CHEROKEE_MACROS_H */
