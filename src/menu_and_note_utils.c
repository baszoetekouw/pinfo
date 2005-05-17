
#include "common_includes.h"

RCSID ("$Id$")

     void
       freeindirect ()
{
  if (indirect)
    {
      xfree (indirect);
      indirect = 0;
    }
  IndirectEntries = 0;
}
void
freetagtable ()
{
  if (tag_table)
    {
      xfree (tag_table);
      tag_table = 0;
    }
  TagTableEntries = 0;
}


void
getnextnode (char *type, char *node)	/* read the `Next:' header entry */
{
  int j, coloncount = 0;
#ifndef ___USE_STATIC___
  char *tmp = xmalloc (strlen (type) + 1);
#else
  static char tmp[1024 + 1];
#endif
  char *wsk;
  strcpy (tmp, type);
  wsk = strstr (tmp, "Next: ");
  if (wsk == 0)
    {
      strcpy (node, ERRNODE);
      return;
    }
  for (j = 6; wsk[j] != 0; j++)
    {
      if ((wsk[j] == ',') || (wsk[j] == '\n'))
	{
	  wsk[j] = 0;
	  strcpy (node, wsk + 6);
#ifndef ___USE_STATIC___
	  xfree (tmp);
#endif
	  return;
	}
    }
#ifndef ___USE_STATIC___
  xfree (tmp);
#endif
}

void
getprevnode (char *type, char *node)	/* read the `Prev:' header entry */
{
  int j, coloncount = 0;
#ifndef ___USE_STATIC___
  char *tmp = xmalloc (strlen (type) + 1);
#else
  static char tmp[1024 + 1];
#endif
  char *wsk;
  strcpy (tmp, type);
  wsk = strstr (tmp, "Prev: ");
  if (wsk == 0)
    {
      strcpy (node, ERRNODE);
      return;
    }
  for (j = 6; wsk[j] != 0; j++)
    {
      if ((wsk[j] == ',') || (wsk[j] == '\n'))
	{
	  wsk[j] = 0;
	  strcpy (node, wsk + 6);
#ifndef ___USE_STATIC___
	  xfree (tmp);
#endif
	  return;
	}
    }
#ifndef ___USE_STATIC___
  xfree (tmp);
#endif
}

void
getupnode (char *type, char *node)	/* read the `Up:' header entry */
{
  int j, coloncount = 0;
#ifndef ___USE_STATIC___
  char *tmp = xmalloc (strlen (type) + 1);
#else
  static char tmp[1024 + 1];
#endif
  char *wsk;
  strcpy (tmp, type);
  wsk = strstr (tmp, "Up: ");
  if (wsk == 0)
    {
      strcpy (node, ERRNODE);
      return;
    }
  for (j = 4; wsk[j] != 0; j++)
    {
      if ((wsk[j] == ',') || (wsk[j] == '\n'))
	{
	  wsk[j] = 0;
	  strcpy (node, wsk + 4);
#ifndef ___USE_STATIC___
	  xfree (tmp);
#endif
	  return;
	}
    }
#ifndef ___USE_STATIC___
  xfree (tmp);
#endif
}


void
getnodename (char *type, char *node)	/* read the `Node:' header entry */
{
  int j, coloncount = 0;
#ifndef ___USE_STATIC___
  char *tmp = xmalloc (strlen (type) + 1);
#else
  static char tmp[1024 + 1];
#endif
  char *wsk;
  strcpy (tmp, type);
  wsk = strstr (tmp, "Node: ");
  if (wsk == 0)
    {
      strcpy (node, ERRNODE);
      return;
    }
  for (j = 6; wsk[j] != 0; j++)
    {
      if ((wsk[j] == ',') || (wsk[j] == '\n'))
	{
	  wsk[j] = 0;
	  strcpy (node, wsk + 6);
#ifndef ___USE_STATIC___
	  xfree (tmp);
#endif
	  return;
	}
    }
#ifndef ___USE_STATIC___
  xfree (tmp);
#endif
}
