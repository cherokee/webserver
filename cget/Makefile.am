## Cherokee: src/cget/Makefile.am  -*- makefile -*-

INCLUDES = -I. \
-DCHEROKEE_COMPILATION \
-I${top_srcdir} \
-I${top_srcdir}/cherokee \
$(TLS_CFLAGS) \
$(LARGEFILE_CFLAGS)


cget_LDADD = \
$(TLS_LIBS) \
../cherokee/libcherokee-client.la 

cget_SOURCES = \
main.c

bin_PROGRAMS = cget
