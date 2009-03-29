#include <u.h>
#include <libc.h>
#include <thread.h>
#include <ctype.h>

#define exit(x) threadexitsall((x) ? "whoops" : nil)
#define size_t ulong
