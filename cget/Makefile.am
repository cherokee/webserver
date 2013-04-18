## Cherokee: src/cget/Makefile.am  -*- makefile -*-

AM_CPPFLAGS = -I. \
-I${top_srcdir} \
-I${top_srcdir}/cherokee \
$(PTHREAD_CFLAGS)

bin_PROGRAMS = cget

cget_LDADD = \
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
