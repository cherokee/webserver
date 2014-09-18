Cherokee Web Server
===================

Web site
--------

Visit our main website for the latest updates: www.cherokee-project.com_.

.. _www.cherokee-project.com: http://www.cherokee-project.com

Compiling from source
---------------------

Building from the repository
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To build from the repository, you will require ``autoconf``, ``automake``, and
``libtool`` tools to be available, typically available on your distribution
either by default or by running the following on Debian-based systems::

    sudo apt-get install autoconf automake libtool

or, for Yum-based systems, such as RedHat, CentOS or Fedora::

    sudo yum install autoconf automake libtool

or, for Pacman-based systems such as ArchLinux::

    sudo pacman -Sy autoconf automake libtool

To check out the code from GitHub, do the following::

    git clone http://github.com/cherokee/webserver.git

Once cloned, ``cd`` into the resulting source directory and run 
``./autogen.sh`` to set up the environment and generate ``./configure``::

    cd webserver
    ./autogen.sh [options]
    make
    make install

``./autogen.sh`` will accept any optional parameters otherwise typically passed
to ``./configure``; in doing so you can avoid needing to run ``./configure``
separately.

Several examples of using ``./autogen.sh`` follow shortly in the
`Frequently Asked Questions (FAQ)`_ section.

Building from a tar.gz file
^^^^^^^^^^^^^^^^^^^^^^^^^^^

After downloading (likely from http://www.cherokee-project.com/downloads.html)
and extracting, you should run ``./configure``, with options as appropriate.
The standard options are documented in the ``INSTALL`` file.  Typically,
the most interesting options are:

* the usual ``--prefix=/usr``
* ``--localstatedir=/var``
* ``--sysconfdir=/etc``

After running ``./configure``, issues the ``make`` command, and then ``make
install`` (excuting this last command as root if the destination permissions
require that).

Tying this all together will result in commands like the following::

    wget http://www.cherokee-project.com/download/trunk/cherokee-latest-snapshot.tar.gz
    tar xf cherokee-latest-snapshot.tar.gz
    cd cherokee-latest-snapshot
    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
    make
    sudo make install

The exact options passed to ``./configure`` can vary based upon your
configuration.


License
-------

Cherokee is released under GPL v2. Read the ``COPYING`` file for more
information.


Mailing lists
-------------

There are several mailing lists available for Cherokee and they are listed
at: 

    http://lists.cherokee-project.com/

The main mailing list, where questions should be sent and general
discussion takes place, is:

    http://lists.cherokee-project.com/listinfo/cherokee

There are also a few technical mailing lists. Developers and package
maintainers should subscribe to these mailing lists as well as the main mailing
list:

    http://lists.cherokee-project.com/listinfo/cherokee-dev

    http://lists.cherokee-project.com/listinfo/cherokee-commits

The mailing lists' archives are available at:

    http://lists.cherokee-project.com/pipermail/cherokee/

Don't hesitate to subscribe and contribute to any of the mailing lists!


IRC channel
-----------

Communicate with the Cherokee community via `IRC
<irc://irc.freenode.net/#cherokee>`_:

    irc.freenode.net, channel #cherokee


Frequently Asked Questions (FAQ)
--------------------------------

Here is a list of the most frequently asked questions regarding
compilation and similar topics:

How to compile it
^^^^^^^^^^^^^^^^^

::

   ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
   make

How to create dynamic modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Modules are created dynamically by default.

How to configure the module xyz to be linked statically
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-static-module=xyz

How to build everyhing statically
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-static-module=all

How to build a MacOS X binary package
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    ./autogen.sh --prefix=/usr/local --with-wwwroot=/Library/WebServer/Documents --with-wwwuser=www --with-wwwgroup=www --with-mysql=no --with-ffmpeg=no --with-ldap=no --enable-beta
    make -j8
    packages/osx/build.py

Development
^^^^^^^^^^^

::

    ./autogen.sh --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-static-module=all --enable-static --enable-shared=no --with-mysql=no --with-ffmpeg=no --with-ldap=no --enable-beta --enable-trace --enable-backtraces --enable-maintainer-mode
    make V=1 CFLAGS="-ggdb3 -O0" -j8

How to create a self signed certificate for TLS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    openssl req -days 1000 -new -x509 -nodes -out /etc/cherokee/ssl/cherokee.pem -keyout /etc/cherokee/ssl/cherokee.pem

How to create a release .tar.gz
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    git clone http://github.com/cherokee/webserver.git
    cd webserver
    ./autogen.sh
    make dist-gzip

The resulting file will be created in the current directory and will be
a ``.tar.gz`` archive.
