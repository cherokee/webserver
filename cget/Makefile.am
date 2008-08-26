## Cherokee: src/cget/Makefile.am  -*- makefile -*-

INCLUDES = -I. \
-I${top_srcdir} \
-I${top_srcdir}/cherokee \
$(TLS_CFLAGS) \
$(PTHREAD_CFLAGS) \
$(LARGEFILE_CFLAGS)

bin_PROGRAMS = cget

cget_LDADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
../cherokee/libcherokee-base.la \
../cherokee/libcherokee-client.la 

cget_SOURCES = \
main.c \
proxy.h \
proxy.c

# Man pag
man_MANS = \
cget.1

EXTRA_DIST = \
$(man_MANS)
