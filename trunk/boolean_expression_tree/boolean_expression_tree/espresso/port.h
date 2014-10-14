#ifndef PORT_H
#define PORT_H

/*
 * int32 should be defined as the most economical sized integer capable of
 * holding a 32 bit quantity
 * int16 should be similarly defined
 */

/* AB Stuff */

typedef int (* qsort_compare_func)(const void *, const void *);
typedef long int32;
typedef int int16;


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#undef HUGE
#include <math.h>
#include <signal.h>

#if defined(ultrix) && defined(SIGLOST)
#define ultrix3
#else
#define ultrix2
#endif

#if defined(sun) && defined(FD_SETSIZE)
#define sunos4
#else
#define sunos3
#endif

#if defined(sequent) || defined(news800)
#define LACK_SYS5
#endif

#if defined(ultrix3) || defined(sunos4)
#define SIGNAL_FN	void
#else
/* sequent, ultrix2, 4.3BSD (vax, hp), sunos3 */
#define SIGNAL_FN	int
#endif

/* Some systems have 'fixed' certain functions which used to be int */
#if defined(ultrix) || defined(SABER) || defined(hpux) || defined(aiws) || defined(apollo) || defined(__STDC__)
#define VOID_HACK void
#else
#define VOID_HACK int
#endif

#ifndef NULL
#define NULL 0
#endif /* NULL */

/*
 * CHARBITS should be defined only if the compiler lacks "unsigned char".
 * It should be a mask, e.g. 0377 for an 8-bit machine.
 */

#ifndef CHARBITS
#	define	UNSCHAR(c)	((unsigned char)(c))
#else
#	define	UNSCHAR(c)	((c)&CHARBITS)
#endif

#define SIZET int

#ifdef __STDC__
#define CONST const
#define VOIDSTAR   void *
#else
#define CONST
#define VOIDSTAR   char *
#endif /* __STDC__ */


/* Some machines fail to define some functions in stdio.h */
#ifndef __STDC__
extern FILE *popen();
extern int pclose();
#endif /* __STDC__ */


/* some call it strings.h, some call it string.h; others, also have memory.h */
#ifdef __STDC__
#include <string.h>
#else
/* ANSI C string.h -- 1/11/88 Draft Standard */
extern char *strcpy(), *strncpy(), *strcat(), *strncat(), *strerror();
extern char *strpbrk(), *strtok(), *strchr(), *strrchr(), *strstr();
extern int strcoll(), strxfrm(), strncmp(), strlen(), strspn(), strcspn();
extern char *memmove(), *memccpy(), *memchr(), *memcpy(), *memset();
extern int memcmp(), strcmp();
#endif /* __STDC__ */

/* assertion macro */

#ifndef assert
#ifdef __STDC__
#include <assert.h>
#else
#ifndef NDEBUG
#define assert(ex) {\
    if (! (ex)) {\
	(void) fprintf(stderr, "Assertion failed: file %s, line %d\n",\
	    __FILE__, __LINE__);\
	(void) fflush(stdout);\
	abort();\
    }\
}
#else
#define assert(ex) {;}
#endif
#endif
#endif


#endif /* PORT_H */

