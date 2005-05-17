#ifndef _SIG_H
#define _SIG_H

#ifndef sigmask
/* from bash */
#define sigmask(x) (1 << ((x)-1))
/* from glibc */
/* #define sigmask(sig)  (((sigset_t) 1) << ((sig) - 1)) */
#endif /* HAVE_SIGMASK */

#if !defined (SIG_BLOCK)
#define SIG_UNBLOCK   1
#define SIG_BLOCK     2
#define SIG_SETMASK   3
#endif /* SIG_BLOCK */

/* #define BLOCK_SIGNAL(sig, nvar, ovar) \
   sigemptyset (&nvar); \
   sigaddset (&nvar, sig); \
   sigemptyset (&ovar); \
   sigprocmask (SIG_BLOCK, &nvar, &ovar) */

#ifndef HAVE_SIGBLOCK
int sigblock (int mask);
#endif /* HAVE_SIGBLOCK */

#endif /* _SIG_H */
