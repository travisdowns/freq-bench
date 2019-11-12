#ifndef SI_UTIL_H_
#define SI_UTIL_H_

#ifndef DEBUG
#define DEBUG 0
#endif

/*
 * dassert is like assert but only enabled if DEBUG is 1
 */
#if DEBUG
#define dassert assert
#else
#define dassert(...)
#endif

#endif // #ifndef SI_UTIL_H_
