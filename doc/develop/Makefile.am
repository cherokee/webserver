EXTRA_DIST = \
Intro.txt \
doxygen.cfg \
doxygen.cfg.in \
Benchmark-0.4.3-pre7.txt \
draft-coar-cgi-v11-03.txt \
rfc1952.txt \
rfc2616.txt \
rfc2817.txt


html/index.html:
	doxygen doxygen.cfg

doc: html/index.html 

CLEANFILES = html/* latex/*

