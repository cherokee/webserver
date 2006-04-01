## Cherokee: src/bench/Makefile.am  -*- makefile -*-

INCLUDES = \
-DCHEROKEE_COMPILATION \
-I. -I${top_srcdir}/cherokee \
$(TLS_CFLAGS) \
$(PTHREAD_CFLAGS) \
$(LARGEFILE_CFLAGS)


cget_LDADD = \
../cherokee/libcherokee-client.la \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS)

cget_SOURCES = \
main.c

bin_PROGRAMS = cget
