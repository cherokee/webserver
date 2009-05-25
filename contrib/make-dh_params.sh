#!/bin/sh

set -e
gensizes="512 1024 2048 4096"

gendhparam() {
	typeset size="$1"
	typeset outfile="$2"

	openssl dhparam -C -2 "$size" | sed -e '1 i/* openssl dhparam -C -2 '"$size"' */
' -e 's/^DH \*get_dh/static &/' -e '/-----BEGIN DH PARAMETERS-----/ { i/*
}' -e '$ {a */
}' > "$outfile"
}

usage() {
	echo "This script must be ran from the contrib dir in cherokee source directory."
	echo "It will overwrite the files cherokee/cryptor_libssl_dh_{512,1024,2048,4096}.c"
	exit 1
}

if [ "$(basename $(pwd))" != "contrib" -o ! -d ../cherokee ] ; then
	usage
fi

for ksize in $gensizes ; do
	gendhparam "$ksize" "../cherokee/cryptor_libssl_dh_$ksize.c"
done
