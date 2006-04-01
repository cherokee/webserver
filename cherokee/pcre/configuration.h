/* If you are compiling for a system that needs some magic to be inserted
before the definition of an exported function, define this macro to contain the
relevant magic. It apears at the start of every exported function. */

#define EXPORT

/* The value of NEWLINE determines the newline character. The default is to
leave it up to the compiler, but some sites want to force a particular value.
On Unix systems, "configure" can be used to override this default. */

#ifndef NEWLINE
#define NEWLINE '\n'
#endif

/* The value of LINK_SIZE determines the number of bytes used to store
links as offsets within the compiled regex. The default is 2, which allows for
compiled patterns up to 64K long. This covers the vast majority of cases.
However, PCRE can also be compiled to use 3 or 4 bytes instead. This allows for
longer patterns in extreme cases. On Unix systems, "configure" can be used to
override this default. */

#ifndef LINK_SIZE
#define LINK_SIZE   2
#endif

/* The value of MATCH_LIMIT determines the default number of times the match()
function can be called during a single execution of pcre_exec(). (There is a
runtime method of setting a different limit.) The limit exists in order to
catch runaway regular expressions that take for ever to determine that they do
not match. The default is set very large so that it does not accidentally catch
legitimate cases. On Unix systems, "configure" can be used to override this
default default. */

#ifndef MATCH_LIMIT
#define MATCH_LIMIT 10000000
#endif

/* When calling PCRE via the POSIX interface, additional working storage is
required for holding the pointers to capturing substrings because PCRE requires
three integers per substring, whereas the POSIX interface provides only two. If
the number of expected substrings is small, the wrapper function uses space on
the stack, because this is faster than using malloc() for each call. The
threshold above which the stack is no longer use is defined by POSIX_MALLOC_
THRESHOLD. On Unix systems, "configure" can be used to override this default.
*/

#ifndef POSIX_MALLOC_THRESHOLD
#define POSIX_MALLOC_THRESHOLD 10
#endif
