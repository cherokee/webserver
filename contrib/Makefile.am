## Cherokee: Makefile.am  -*- makefile -*-

cherokee: cherokee.pre
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@

EXTRA_DIST = \
cherokee.pre \
bin2buffer.py \
05to06.py \
06to07.py \
07to08.py \
08to09.py \
09to010.py \
011to098.py \
098to099.py \
0999to09910.py \
tracelor.py \
make-cert.sh

CLEANFILES = \
cherokee \
server.pem \
rand.dat \
cakey.key \
cakey.csr \
cakey.pem \
make-dh_params.sh

all-local: cherokee
