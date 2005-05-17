#include "common_includes.h"

RCSID ("$Id$")

#include <termios.h>
#include <sys/ioctl.h>

     void
       handle_crash (int signum)
{
  closeprogram ();
  fprintf (stderr, "Caught signal %d, bye!\n", signum);
  if (signum == SIGSEGV)
    perror ("pinfo: crash with");
  exit (1);
}

void
handle_window_resize (int signum)
{
  winchanged = 1;
  ungetch (keys.refresh_1);
  signal (SIGWINCH, handle_window_resize);
}

void
signal_handler ()
{
  signal (SIGINT, handle_crash);	/* handle ^C */
  signal (SIGTERM, handle_crash);	/* handle soft kill */
  signal (SIGSEGV, handle_crash);	/* handle seg. fault */
  signal (SIGHUP, handle_crash);	/* handle hup signal */
#ifdef SIGWINCH
  signal (SIGWINCH, handle_window_resize);
#endif
  sigblock (sigmask (SIGPIPE));	/* block broken pipe signal */
}
