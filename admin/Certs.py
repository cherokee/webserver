# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

import os
import CTK

from util import run

SSL_CONFIG = """
[ req ]
default_bits = 1024
encrypt_key = yes
distinguished_name = req_dn
x509_extensions = cert_type
prompt = no

[ req_dn ]
# Organizational Unit Name (eg. section)
OU=Cherokee Web Server

# Common Name (*.example.com is also possible)
CN=%(domain)s

# E-mail contact
emailAddress=root@%(domain)s

[ cert_type ]
nsCertType = server
"""


def create_selfsigned_cert (dir_path, filename, domain, bits=1024, days=9999):
    conf = SSL_CONFIG %(locals())

    # Write the OpenSSL config file
    conf_fp = os.path.join (dir_path, "tmp_ssl.conf")

    f = open (conf_fp, 'w+')
    f.write (conf)
    f.close()

    # Create the self signed cert
    crt_fp = os.path.join (dir_path, "%s.crt"%(filename))
    key_fp = os.path.join (dir_path, "%s.key"%(filename))

    cmd = "openssl req -new -x509 -nodes -out %(crt_fp)s -keyout %(key_fp)s -config %(conf_fp)s" %(locals())
    re = run (cmd, stdout=False, stderr=True, retcode=True)

    os.unlink (conf_fp)

    if re['retcode'] != 0:
        return re['stderr']


if __name__ == "__main__":
    create_selfsigned_cert ("/tmp", "test")
