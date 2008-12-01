#!/bin/sh

# Generate some random data
rm -f rand.dat
dd if=/dev/random of=rand.dat bs=1024 count=1

# cakey.key
openssl genrsa -out cakey.key -rand rand.dat 2048

# cakey.csr
openssl req -new -key cakey.key -out cakey.csr

# cakey.pem
openssl x509 -req -days 1780 -set_serial 1 -in cakey.csr -signkey cakey.key -out cakey.pem

# server.pem
cat cakey.key cakey.pem > server.pem

# Clean up
rm -f rand.dat
ls -l server.pem
