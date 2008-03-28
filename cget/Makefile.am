## Cherokee: src/cget/Makefile.am  -*- makefile -*-

INCLUDES = -I. \
-I${top_srcdir}/cherokee \
$(TLS_CFLAGS) \
$(PTHREAD_CFLAGS) \
$(LARGEFILE_CFLAGS)

# Man pag
man_MANS = \
cget.1

cget_LDADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
../cherokee/libcherokee-client.la 

cget_SOURCES = \
main.c \
proxy.h \
proxy.c

bin_PROGRAMS = cget
