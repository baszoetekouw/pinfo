/******************************************************************************
* Pinfo is a lynx-style info and manual viewer. It is written by              *
* Przemek Borys <pborys@dione.ids.pl>. Copying policy is GPL                  *
******************************************************************************/

#include "common_includes.h"

RCSID (PKG_VER "$Id$")

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif

     char *version = VERSION;
     int DontHandleWithoutTagTable = 0;

     char *curfile = 0;		/* currently viewed filename */

     char *pinfo_start_node = 0;	/* node specified by --node option */

     void strip_file_from_info_suffix (char *file);	/* 
							 * strip `.info' suffix from
							 * "file" 
							 */
     char *addinfosuffix (char *file);	/* add `.info' suffix to "file" */

     void checksu ();		/* protect against bad, bad macros */

     int
       main (int argc, char *argv[])
{
  int filenotfound = 0;
  char filename[256];
  WorkRVal work_return_value =
  {0, 0};
  int i, userdefinedrc = 0;
  int command_line_option;
  FILE *id = NULL;
  long lines = 0;		/* line count in message */
  char **message = 0;		/* this will hold node's text */
  char *type = 0;		/* this will hold the node's header */
  int tag_table_pos = 1;
  char *tmp;
#ifdef HAVE_GETOPT_LONG
  static struct option long_options[] =
  {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"manual", 0, 0, 'm'},
    {"file", 0, 0, 'f'},
    {"raw-filename", 0, 0, 'r'},
    {"apropos", 0, 0, 'a'},
    {"plain-apropos", 0, 0, 'p'},
    {"cut-man-headers", 0, 0, 'c'},
    {"squeeze-manlines", 0, 0, 's'},
    {"dont-handle-without-tag-table", 0, 0, 'd'},
    {"force-manual-tag-table", 0, 0, 't'},
    {"node", 1, 0, 'n'},
    {"long-manual-links", 0, 0, 'l'},
    {"clear-at-exit", 0, 0, 'x'},
    {"rcfile", 1, 0, 1},	/* no one-letter shortcut :( */
    {0, 0, 0, 0}};
#endif
  signal_handler ();		/* take care of SIGSEGV, SIGTERM, SIGINT */
  searchagain.type = 0;
  searchagain.search = 0;
  initlocale ();
  inithistory ();
  for (i = 1; i < argc; i++)
    if (strncmp (argv[i], "--rcfile", 8) == 0)
      userdefinedrc = 1;
  if (!userdefinedrc)
    parse_config ();		/* read config information */
  if (verbose)
    printf ("Przemek's Info Viewer v%s\n", version);
  if (argc == 1)		/* if no arguments were given */
    {
      id = openinfo ("dir", 0);
      curfile = xmalloc (150);
      strcpy (curfile, "dir");
      strcpy (filename, "dir");
    }
  if ((strlen (argv[0]) >= 3)||(use_manual))
    if ((strstr (argv[0], "man") != NULL)||(use_manual))	/* handle any 'man' alias to 'pinfo' */
      {
	if (verbose)
	  printf (_ ("Looking for man page...\n"));
	strcpy (filename, "");
	for (i = 1; i < argc; i++)	/* pass all arguments to the `man' 
					   command (manhandler calls `man') */
	  {
	    strcat (filename, argv[i]);
	    strcat (filename, " ");
	  }
	exit (handlemanual (filename));
      }



#ifdef HAVE_GETOPT_LONG

/******************************************************************************
* Parse command line options (getopt)                                         *
******************************************************************************/

  do
    {
      command_line_option = getopt_long (argc, argv, "hvmfrapcsdtnlx", long_options, NULL);
      switch (command_line_option)
	{
	case 'x':
	  ClearScreenAtExit = 1;
	  break;
	case 'l':
	  LongManualLinks = 1;
	  break;
	case 'n':
	  if (!optarg)
	    {
	      printf (_ ("--node option used without argument\n"));
	      exit (1);
	    }
	  pinfo_start_node = malloc (strlen (optarg) + 1);
	  strcpy (pinfo_start_node, optarg);
	  break;
	case 1:		/* rcfile */
	  if (!optarg)
	    {
	      printf (_ ("--rcfile option used without argument\n"));
	      exit (1);
	    }
	  rcfile = strdup (optarg);
	  parse_config ();	/* parse user-defined config file */
	  break;
	case 't':
	  ForceManualTagTable = 1;
	  break;
	case 'h':
	  printf (_ ("Usage:\n" \
		     "%s [options] [info|manual]\n" \
		     "Options:\n" \
		     "-h, --help                            help\n" \
		     "-v, --version                         version\n" \
		     "-m, --manual                          use man page\n" \
		"-r, --raw-filename                    use raw filename\n" \
		  "-f, --file                            synonym for -r\n" \
		     "-a, --apropos                         call apropos if nothing found\n" \
	       "-p, --plain-apropos                   call only apropos\n" \
		     "-c, --cut-man-headers                 cut out repeated man headers\n" \
		     "-l, --long-manual-links               use long link names in manuals\n" \
		     "-s, --squeeze-manlines                cut empty lines from manual pages\n" \
		     "-d, --dont-handle-without-tag-table   don't display texinfo pages without tag\n" \
		     "                                      tables\n" \
		     "-t, --force-manual-tag-table          force manual detection of tag table\n" \
	    "-x, --clear-at-exit                   clear screen at exit\n" \
		     "    --node=nodename, --node nodename  jump directly to the node nodename\n" \
	    "    --rcfile=file, --rcfile file      use alternate rcfile\n"),
		  argv[0]);
	  exit (0);
	case 'v':
	  exit (0);
	case 'm':
	  checksu ();
	  if (verbose)
	    printf (_ ("Looking for man page...\n"));
	  strcpy (filename, "");
	  for (i = optind; i < argc; i++)
	    {
	      strcat (filename, argv[i]);
	      strcat (filename, " ");
	    }
	  exit (handlemanual (filename));
	case 'f':
	case 'r':
	  strncpy (filename, argv[argc - 1], 200);
	  checkfilename (filename);	/* security check */
	  addrawpath (filename);	/* add the raw path to searchpath */
	  tmp = filename + strlen (filename) - 1;
	  strip_compression_suffix (filename);	/* later, openinfo automaticaly adds them */
	  while ((tmp > filename) && (*tmp != '/'))
	    tmp--;		/* get basename */
	  if (*tmp == '/')
	    tmp++;
	  id = openinfo (tmp, 0);	/* and try it without '.info' suffix */
	  break;
	case 'a':
	  use_apropos = 1;
	  break;
	case 'p':
	  use_apropos = 1;
	  plain_apropos = 1;
	  strncpy (filename, argv[argc - 1], 200);
	  exit (handlemanual (filename));
	  break;
	case 'c':
	  CutManHeaders = 1;
	  break;
	case 'd':
	  DontHandleWithoutTagTable = 1;
	  break;
	case 's':
	  CutEmptyManLines = 1;
	  break;
	}
    }
  while (command_line_option != EOF);
/*****************************************************************************/
#endif

  checksu ();
  initpaths ();

  if (argc > 1)
    {
#ifdef HAVE_GETOPT_LONG
      if (optind < argc)
	{
	  strncpy (filename, argv[optind], 200);	/* the paths will be searched
							   by openinfo() */
	}
      else
	{
	  strcpy (filename, "dir");
	}

#else
      strncpy (filename, argv[argc - 1], 200);	/* the paths will be searched
						   by openinfo() */
#endif
      if(filename[0]=='(')
        {
          int fnamelen=strlen(filename);
          for(i=0;i<fnamelen;i++)	/* erase the leading '(' */
            filename[i]=filename[i+1];
          for(i=0;filename[i]!=')';i++);
          filename[i]=0;		/* leave the filename part in filename */
          if(!pinfo_start_node)		/* copy the node content to pinfo_start_node */
            {
              pinfo_start_node=strdup(&filename[i+1]);
            }
        }

      checkfilename (filename);	/* security check */

      if((strncmp(filename,"../",3)==0)||	/* autodetect raw filenames */
         (strncmp(filename,"./",2)==0)||
         (filename[0]=='/'))
         {
           addrawpath(filename);
         }
               
      curfile = xmalloc (strlen (filename) + 100);	/* leave some space for
							   `.info' suffix */
      strcpy (curfile, filename);
    }

  if(id == NULL)		/* no rawpath has been opened */
    id = openinfo (filename, 0);

  if (id == NULL)		/* try to lookup the name in dir file */
    {
      id = dirpage_lookup (&type, &message, &lines, filename, &pinfo_start_node);
    }
  if (id == NULL)		/* if still nothing, try to use man page instead */
    {
      printf (_ ("Error: could not open info file, trying manual\n"));
      exit (handlemanual (filename));
    }
  if (seek_indirect (id))	/* search for indirect entries, if any */
    {
      read_item (id, &type, &message, &lines);
      load_indirect (message, lines);
    }


  if (seek_tag_table (id,1) != 2)	/* load tag table if such exists... */
    {
      if (ForceManualTagTable == 0)
	{
	  read_item (id, &type, &message, &lines);
	  load_tag_table (message, lines);
	}
      else
	{
	  if (indirect)
	    create_indirect_tag_table ();
	  else
	    {
	      fseek (id, SEEK_SET, 0);
	      create_tag_table (id);
	    }
	}
    }
  else
    /* ...otherwise try to create one */
    {
      if ((verbose)&&(strcmp(curfile,"dir")))
	printf (_ ("Warning: tag table not found...\n"));
      if (!DontHandleWithoutTagTable)
	{
	  if ((verbose)&&(strcmp(curfile,"dir")))
	    printf (_ ("Trying to create alternate tag table...\n"));
	  create_tag_table (id);
	  if (TagTableEntries < 1)	/* if there weren't found any info
					   entries */
	    {
	      printf (_ ("This doesn't look like info file...\n"));
	      exit (handlemanual (filename));
	    }
	}
      else
	return 1;
    }

  if (pinfo_start_node)
    {
      tag_table_pos = gettagtablepos (pinfo_start_node);
      if (tag_table_pos == -1)
	{
	  printf (_ ("Specified node does not exist...\n"));
	  return 1;
	}
    }
  else
    {
      tag_table_pos = gettagtablepos (FirstNodeName);
    }
  init_curses ();		/* initialize curses screen interface */

  do
    {
      seeknode (tag_table_pos, &id);	/* set seek offset for given node */
      read_item (id, &type, &message, &lines);	/* read the node */

      if (!filenotfound)	/* handle goto/link where no file was found 
				   -- see bellow */
	addinfohistory (curfile, tag_table[tag_table_pos].nodename, -1, -1, -1);
      else
	filenotfound = 0;
      work_return_value = work (&message, &type, &lines, id, tag_table_pos);
      if (work_return_value.node)
	{
	  if (work_return_value.file[0] == 0)	/* no cross-file link selected */
	    {
	      int tmppos = gettagtablepos (work_return_value.node);
	      if (tmppos != -1)
		tag_table_pos = tmppos;
	    }
	  else
	    /* file was specified */
	    {
	      strip_file_from_info_suffix (work_return_value.file);
	      if (strcmp (curfile, work_return_value.file) == 0)
		/* file name was the same with the file currently viewed */
		{
		  int tmppos = gettagtablepos (work_return_value.node);
		  if (tmppos != -1)
		    tag_table_pos = tmppos;
		}
	      else
		/* open new info file */
		{
		  char *tmp;
		  fclose (id);
		  tmp = strdup (work_return_value.file);	/*tmp = addinfosuffix (work_return_value.file); */
		  clearfilenameprefix ();
		  id = openinfo (tmp, 0);
		  xfree (tmp);
		  tmp = 0;
		  if (id == NULL)	/* if the file doesn't exist */
		    {
		      attrset (bottomline);
		      mvhline (maxy - 1, 0, ' ', maxx);
		      mvaddstr (maxy - 1, 0, _ ("File not found. Press any key..."));
		      move (0, 0);
		      attrset (normal);
		      getch ();
		      filenotfound = 1;
		      if (infohistory.length)
			{
			  npos = infohistory.pos[infohistory.length];
			  ncursor = infohistory.cursor[infohistory.length];
			}
		      strip_file_from_info_suffix (curfile);	/* open back the old file */
		      tmp = strdup (curfile);	/*tmp = addinfosuffix (curfile); */
		      id = openinfo (tmp, 0);
		      xfree (tmp);
		      tmp = 0;
		      if (id == NULL)
			{
			  closeprogram ();
			  printf (_ ("Unexpected error.\n"));
			  return 1;
			}
		    }
		  else
		    /* if we succeeded in opening new file */
		    {
		      if (curfile)
			{
			  xfree (curfile);
			  curfile = 0;
			}
		      curfile = xmalloc (strlen (work_return_value.file) + 150);
		      strcpy (curfile, work_return_value.file);
		      freeindirect ();
		      if (seek_indirect (id))	/* find the indirect entry */
			{
			  read_item (id, &type, &message, &lines);	/* read it */
			  load_indirect (message, lines);	/* initialize indirect entries */
			}
		      freetagtable ();	/* free old tag table */
		      if (seek_tag_table (id,0) != 2)	/* search for the new tagtable */
			{
			  if (ForceManualTagTable == 0)		/* if no manual initialization requested, load the tag table */
			    {
			      read_item (id, &type, &message, &lines);
			      load_tag_table (message, lines);
			    }
			  else
			    /* create tag table manually */
			    {
			      if (indirect)
				create_indirect_tag_table ();
			      else
				{
				  fseek (id, SEEK_SET, 0);
				  create_tag_table (id);
				}
			    }
			}
		      else
			/* no tagtable found */
			{
			  if (!DontHandleWithoutTagTable)
			    {
			      TagTableEntries = 0;
			      mvhline (maxy - 1, 0, ' ', maxx);
			      mvaddstr (maxy - 1, 0, _ ("Tag table not found. Trying to create alternate..."));
			      create_tag_table (id);
			      if (TagTableEntries < 1)
				{
				  closeprogram ();
				  printf (_ ("This doesn't look like info file...\n"));
				  return 1;
				}
			    }
			  else
			    return 1;
			}
		      if (work_return_value.node[0] != 0)
			{
			  int tmptagtablepos = gettagtablepos (work_return_value.node);
			  if (tmptagtablepos != -1)
			    tag_table_pos = tmptagtablepos;
			  else
			    tag_table_pos = gettagtablepos (FirstNodeName);
			}
		      else
			tag_table_pos = gettagtablepos (FirstNodeName);

		    }		/* end: open new info file -- file exists */
		}		/* end: open new info file */
	    }			/* end: file name was specified */
	}			/* end: node was specified in work return value */
    }
  while (work_return_value.node);
  fclose (id);
  closeprogram ();
  freelinks ();			/* free's at the end are optional, but look nice :) */
  freeitem (&type, &message, &lines);
  freetagtable ();
  freeindirect ();
  return 0;
}
void
strip_file_from_info_suffix (char *file)
{
  if (strlen (file) > 5)
    {
      if (strcmp (file + strlen (file) - 5, ".info") == 0)
	{
	  file = file + strlen (file) - 5;
	  *file = 0;
	}
    }
}
char *
addinfosuffix (char *info)
{
  char *withsuffix = xmalloc (strlen (info) + 150);
  strcpy (withsuffix, info);
  if (strlen (info) == 3)
    {
      if (strcmp ("dir", info) != 0)
	strcat (withsuffix, ".info");
    }
  else
    strcat (withsuffix, ".info");

  return withsuffix;
}

/* If pinfo was called by root then it should work as nobody.   */
/* This protect us against .pso and .open macros which could    */
/* be used for breaking the system's security.                  */

void
checksu ()
{
  struct passwd *pswd;
  struct group *grwd;

  if (!getegid () || !getgid ())
    {
      grwd = getgrnam (safe_group);
      if (!grwd)
	{
	  if (verbose)
	    {
	      printf (_ ("Security warning: Unable to get GID of group called: %s\n"), safe_group);
	      sleep (1);
	    }
	}
      else
	{
	  if (!getgid () && !getuid ())
	    setgid (grwd->gr_gid);
	  else
	    setegid (grwd->gr_gid);
	}
    }

  if (!geteuid () || !getuid ())
    {
      pswd = getpwnam (safe_user);
      if (!pswd)
	{
	  if (verbose)
	    {
	      printf (_ ("Security warning: Unable to get UID of user called: %s\n"), safe_user);
	      sleep (1);
	    }
	}
      else
	{
	  if (!getuid ())
	    setuid (pswd->pw_uid);
	  else
	    seteuid (pswd->pw_uid);
	}
    }

}
