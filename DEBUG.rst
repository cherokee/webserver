In the past we did some quite extensive work using Coverity and the Static Analyzer of gcc and clang.
In recent bughunting by LogicalTrust an interesting approach using AddressSanitizer was presented for in depth debugging that would only be partly available under Valgrind conditions.
To make memory violations directly visible consider consider compiling Cherokee using:

.. code-block:: bash

   ac_cv_func_realloc_0_nonnull=yes ac_cv_func_malloc_0_nonnull=yes LDFLAGS="-lasan" LDADD="-lasan" CFLAGS="-fsanitize=address -ggdb -O0 -fprofile-arcs -ftest-coverage" ./configure --prefix=`pwd`/bin --enable-trace --enable-static-module=all --enable-static --enable-shared=no



Or the variant without threading:

.. code-block:: bash

   ac_cv_func_realloc_0_nonnull=yes ac_cv_func_malloc_0_nonnull=yes LDFLAGS="-lasan" LDADD="-lasan" CFLAGS="-fsanitize=address -ggdb -O0 -fprofile-arcs -ftest-coverage" ./configure --prefix=`pwd`/bin --enable-trace --enable-static-module=all --enable-static --enable-shared=no --disable-pthread
