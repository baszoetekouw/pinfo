/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
 *  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
 *  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA
 ***************************************************************************/

#include "common_includes.h"

#ifdef HAVE_GETOPT_LONG
 #include <getopt.h>
#endif

char *version = PACKAGE_VERSION;

/* currently viewed filename */
char *curfile = 0;

/* node specified by --node option */
char *pinfo_start_node = 0;

/* strip `.info' suffix from  "file" */
void strip_file_from_info_suffix(char *file);
/* add `.info' suffix to "file" */
char *addinfosuffix(char *file);

/* protect against bad, bad macros */
void checksu();

int
main(int argc, char *argv[])
{
	int filenotfound = 0;
	char filename[256];
	WorkRVal work_return_value =
	{0, 0};
	int i, userdefinedrc = 0;
	int command_line_option;
	FILE *id = NULL;
	/* line count in message */
	unsigned long lines = 0;
	/* this will hold node's text */
	char **message = 0;
	/* this will hold the node's header */
	char *type = 0;
	int tag_table_pos = 1;
	char *file_name_force = NULL;
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
		/* no one-letter shortcut :( */
		{"rcfile", 1, 0, 1},
		{0, 0, 0, 0}};
#endif
		/* take care of SIGSEGV, SIGTERM, SIGINT */
		signal_handler();
		searchagain.type = 0;
		searchagain.search = 0;
		initlocale();
		inithistory();
		for (i = 1; i < argc; i++)
			if (strncmp(argv[i], "--rcfile", 8) == 0)
				userdefinedrc = 1;
		/* read config information */
		if (!userdefinedrc)
			parse_config();
		if (verbose)
			printf("Przemek's Info Viewer v%s\n", version);
		/* if no arguments were given */
		if (argc == 1)
		{
			id = openinfo("dir", 0);
			curfile = xmalloc(150);
			strcpy(curfile, "dir");
			strcpy(filename, "dir");
		}
		if ((strlen(argv[0]) >= 3)||(use_manual))
			/* handle any 'man' alias to 'pinfo' */
			if ((strstr(argv[0], "man") != NULL)||(use_manual))
			{
				if (verbose)
					printf(_("Looking for man page...\n"));
				strcpy(filename, "");
				/*
				 * pass all arguments to the `man' command(manhandler calls
				 * `man')
				 */
				for (i = 1; i < argc; i++)
				{
					strncat(filename, argv[i], sizeof(filename)-strlen(filename)-2);
					strncat(filename, " ", 2);
				}
				exit(handlemanual(filename));
			}



#ifdef HAVE_GETOPT_LONG

		/******************************************************************************
		 * Parse command line options(getopt)                                         *
		 ******************************************************************************/

		do
		{
			char *tmp;
			command_line_option = getopt_long(argc, argv,
					"hvmfrapcsdtn:lx", long_options, NULL);
			switch(command_line_option)
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
						printf(_("--node option used without argument\n"));
						exit(1);
					}
					pinfo_start_node = malloc(strlen(optarg) + 1);
					strcpy(pinfo_start_node, optarg);
					break;
				/* rcfile */
				case 1:
					if (!optarg)
					{
						printf(_("--rcfile option used without argument\n"));
						exit(1);
					}
					rcfile = strdup(optarg);
					/* parse user-defined config file */
					parse_config();
					break;
				case 't':
					ForceManualTagTable = 1;
					break;
				case 'h':
					printf(_("Usage:\n" \
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
					exit(0);
				case 'v':
					// If the `verbose` option was not enabled, then the version has not been shown
					// and it has to be shown now
					if (!verbose)
						printf("pinfo v%s\n", version);
					exit(0);
				case 'm':
					checksu();
					if (verbose)
						printf(_("Looking for man page...\n"));
					strcpy(filename, "");
					for (i = optind; i < argc; i++)
					{
						strncat(filename, argv[i], sizeof(filename)-strlen(filename)-2);
						strncat(filename, " ", 2);
					}
					exit(handlemanual(filename));
				case 'f':
				case 'r':
					strncpy(filename, argv[argc - 1], 200);
					/* security check */
					checkfilename(filename);
					/* add the raw path to searchpath */
					addrawpath(filename);
					tmp = filename + strlen(filename) - 1;
					/* later, openinfo automaticaly adds them */
					strip_compression_suffix(filename);
					/* get basename */
					while ((tmp > filename) &&(*tmp != '/'))
						tmp--;
					if (*tmp == '/')
						tmp++;
					/* and try it without '.info' suffix */
					id = openinfo(tmp, 0);
					break;
				case 'a':
					use_apropos = 1;
					break;
				case 'p':
					use_apropos = 1;
					plain_apropos = 1;
					strncpy(filename, argv[argc - 1], 200);
					exit(handlemanual(filename));
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
				case '?':
					exit(1);
			}
		}
		while (command_line_option != EOF);
		/***************************************************************/
#endif

		checksu();
		initpaths();

		if (argc > 1)
		{
#ifdef HAVE_GETOPT_LONG
			if (optind < argc)
			{
				/* the paths will be searched by openinfo() */
				strncpy(filename, argv[optind], 200);
			}
			else
			{
				strcpy(filename, "dir");
			}

#else
			/* the paths will be searched by openinfo() */
			strncpy(filename, argv[argc - 1], 200);
#endif
			if (filename[0]=='(')
			{
				int fnamelen=strlen(filename);
				/* erase the leading '(' */
				for (i=0;i<fnamelen;i++)
					filename[i]=filename[i+1];
				for (i=0;filename[i]!=')';i++);
				/* leave the filename part in filename */
				filename[i]=0;
				/* copy the node content to pinfo_start_node */
				if (!pinfo_start_node)
				{
					pinfo_start_node=strdup(&filename[i+1]);
				}
			}

			/* security check */
			checkfilename(filename);

			/* autodetect raw filenames */
			if ((strncmp(filename,"../",3)==0)||
					(strncmp(filename,"./",2)==0)||
					(filename[0]=='/'))
			{
				addrawpath(filename);
			}

			/* leave some space for `.info' suffix */
			curfile = xmalloc(strlen(filename) + 100);
			strcpy(curfile, filename);
		}

		/* no rawpath has been opened */
		if (id == NULL)
			id = openinfo(filename, 0);

		/* try to lookup the name in dir file */
		if (id == NULL)
		{
			id = dirpage_lookup(&type, &message, &lines, filename, &pinfo_start_node);
		}
		/* if still nothing, try to use man page instead */
		if (id == NULL)
		{
			printf(_("Error: could not open info file, trying manual\n"));
			exit(handlemanual(filename));
		}
		/* search for indirect entries, if any */
		if (seek_indirect(id))
		{
			read_item(id, &type, &message, &lines);
			load_indirect(message, lines);
		}


		/* load tag table if such exists... */
		if (seek_tag_table(id,1) != 2)
		{
			if (ForceManualTagTable == 0)
			{
				read_item(id, &type, &message, &lines);
				load_tag_table(message, lines);
			}
			else
			{
				if (indirect)
					create_indirect_tag_table();
				else
				{
					fseek(id, SEEK_SET, 0);
					create_tag_table(id);
				}
			}
		}
		else /* ...otherwise try to create one */
		{
			if ((verbose)&&(strcmp(curfile,"dir")))
				printf(_("Warning: tag table not found...\n"));
			if (!DontHandleWithoutTagTable)
			{
				if ((verbose)&&(strcmp(curfile,"dir")))
					printf(_("Trying to create alternate tag table...\n"));
				create_tag_table(id);
				/* if there weren't found any info entries */
				if (TagTableEntries < 1)
				{
					printf(_("This doesn't look like info file...\n"));
					exit(handlemanual(filename));
				}
			}
			else
				return 1;
		}

		if (pinfo_start_node)
		{
			tag_table_pos = gettagtablepos(pinfo_start_node);
			if (tag_table_pos == -1)
			{
				printf(_("Specified node does not exist...\n"));
				return 1;
			}
		}
		else
		{
			tag_table_pos = gettagtablepos(FirstNodeName);
		}
		/* initialize curses screen interface */
		init_curses();

		do
		{
			/* set seek offset for given node */
			if (seeknode(tag_table_pos, &id)!=0)
			{
				tag_table_pos = gettagtablepos(FirstNodeName);
				if (seeknode(tag_table_pos, &id)!=0)
				{
					exit(-1);
				}
			}
			/* read the node */
			read_item(id, &type, &message, &lines);

			/* handle goto/link where no file was found -- see below */
			if (!filenotfound)
			{
				addinfohistory(curfile, tag_table[tag_table_pos].nodename,
						-1, -1, -1);
			}
			else
				filenotfound = 0;

			/* this might have been allocated in the previous iteration */
			if (file_name_force!=NULL)
			{
				xfree(file_name_force);
				file_name_force = NULL;
			}

			/* check if we really found the node we were looking for
			 * (don't do this for tag tables we manually created, as this
			 * might cause a loop if somethign goes wrong)
			 *
			 * the entire handling of file_name_force and work_return_value.file is a
			 * big hack, but it'll have to do for now, until the entire work
			 * loop thing is rewritten.
			 */
			if ( (ForceManualTagTable==0)
				 && (check_node_name(work_return_value.node, type) == 0 ))
			{
				/* Oops, we found the wrong node! */

				/* display error message to make the user aware of
				 * the broken info page
				 */
				char msg[81];
				snprintf(msg, 81, "%s (%s)",
						_("Tag table is corrupt, trying to fix..."),
						_("press a key to continue") );
				attrset(bottomline);
				mvhline(maxy - 1, 0, ' ', maxx);
				mvaddstr(maxy - 1, 0, msg);
				move(0, 0);
				attrset(normal);
				getch();

				/* We found another node than we were looking for, so the
				 * tag table must be corrupt. Try to fix it by manually
				 * creating tag tables and... */
				ForceManualTagTable = 1;

				/* forcing the current file to reload by seting
				 * work_return_value.file to the current file, and the current
				 * file to \0
				 */
				if (file_name_force) xfree(file_name_force);
				file_name_force = xmalloc( strlen(curfile)+1 ); /* freed below */
				strcpy(file_name_force, curfile);
				curfile[0] = '\0';
				work_return_value.file = file_name_force;

				/* remove this try from the history stack */
				dellastinfohistory();
			}
			else
			{
				/* everything went fine, so display the node and wait for
				 * key events and stuff */
				work_return_value = work(&message, &type, &lines, id, tag_table_pos);
			}
			if (work_return_value.node)
			{
				/* no cross-file link selected */
				if (work_return_value.file[0] == 0)
				{
					int tmppos = gettagtablepos(work_return_value.node);
					if (tmppos != -1)
						tag_table_pos = tmppos;
				}
				else /* file was specified */
				{
					strip_file_from_info_suffix(work_return_value.file);
					/* file name was the same with the file currently viewed */
					if (strcmp(curfile, work_return_value.file) == 0)
					{
						int tmppos = gettagtablepos(work_return_value.node);
						if (tmppos != -1)
							tag_table_pos = tmppos;
					}
					else /* open new info file */
					{
						char *tmp;
						fclose(id);
						/*tmp = addinfosuffix(work_return_value.file); */
						tmp = strdup(work_return_value.file);
						clearfilenameprefix();
						id = openinfo(tmp, 0);

						/* try to lookup the name in dir file */
						if (id == NULL)
						{
							id = dirpage_lookup(&type, &message, &lines, tmp, &pinfo_start_node);
						}

						xfree(tmp);
						tmp = 0;
						/* if the file doesn't exist */
						if (id == NULL)
						{
							attrset(bottomline);
							mvhline(maxy - 1, 0, ' ', maxx);
							mvaddstr(maxy - 1, 0, _("File not found. Press any key..."));
							move(0, 0);
							attrset(normal);
							getch();
							filenotfound = 1;
							if (infohistory.length)
							{
								npos = infohistory.pos[infohistory.length];
								ncursor = infohistory.cursor[infohistory.length];
							}
							/* open back the old file */
							strip_file_from_info_suffix(curfile);
							/*tmp = addinfosuffix(curfile); */
							tmp = strdup(curfile);
							id = openinfo(tmp, 0);
							xfree(tmp);
							tmp = 0;
							if (id == NULL)
							{
								closeprogram();
								printf(_("Unexpected error.\n"));
								return 1;
							}
						}
						else /* if we succeeded in opening new file */
						{
							if (curfile)
							{
								xfree(curfile);
								curfile = 0;
							}
							curfile = xmalloc(strlen(work_return_value.file) + 150);
							strcpy(curfile, work_return_value.file);
							freeindirect();
							/* find the indirect entry */
							if (seek_indirect(id))
							{
								/* read it */
								read_item(id, &type, &message, &lines);
								/* initialize indirect entries */
								load_indirect(message, lines);
							}
							/* free old tag table */
							freetagtable();
							/* search for the new tagtable */
							if (seek_tag_table(id,0) != 2)
							{
								/*
								 * if no manual initialization requested,
								 * load the tag table
								 */
								if (ForceManualTagTable == 0)
								{
									read_item(id, &type, &message, &lines);
									load_tag_table(message, lines);
								}
								else /* create tag table manually */
								{
									if (indirect)
										create_indirect_tag_table();
									else
									{
										fseek(id, SEEK_SET, 0);
										create_tag_table(id);
									}
								}
							}
							else /* no tagtable found */
							{
								if (!DontHandleWithoutTagTable)
								{
									TagTableEntries = 0;
									mvhline(maxy - 1, 0, ' ', maxx);
									mvaddstr(maxy - 1, 0, _("Tag table not found. Trying to create alternate..."));
									create_tag_table(id);
									if (TagTableEntries < 1)
									{
										closeprogram();
										printf(_("This doesn't look like info file...\n"));
										return 1;
									}
								}
								else
									return 1;
							}
							if (work_return_value.node[0] != 0)
							{
								int tmptagtablepos = gettagtablepos(work_return_value.node);
								if (tmptagtablepos != -1)
									tag_table_pos = tmptagtablepos;
								else
									tag_table_pos = gettagtablepos(FirstNodeName);
							}
							else
								tag_table_pos = gettagtablepos(FirstNodeName);

						}		/* end: open new info file -- file exists */
					}		/* end: open new info file */
				}			/* end: file name was specified */
			}			/* end: node was specified in work return value */
		}
		while (work_return_value.node);
		fclose(id);
		closeprogram();
		/* free's at the end are optional, but look nice :) */
		freelinks();
		freeitem(&type, &message, &lines);
		freetagtable();
		freeindirect();
		return 0;
}

void
strip_file_from_info_suffix(char *file)
{
	if (strlen(file) > 5)
	{
		if (strcmp(file + strlen(file) - 5, ".info") == 0)
		{
			file = file + strlen(file) - 5;
			*file = 0;
		}
	}
}

char *
addinfosuffix(char *info)
{
	char *withsuffix = xmalloc(strlen(info) + 150);
	strcpy(withsuffix, info);
	if (strlen(info) == 3)
	{
		if (strcmp("dir", info) != 0)
			strcat(withsuffix, ".info");
	}
	else
		strcat(withsuffix, ".info");

	return withsuffix;
}

/*
 * If pinfo was called by root then it should work as nobody.
 * This protect us against .pso and .open macros which could
 * be used for breaking the system's security.
 */
void
checksu()
{
	struct passwd *pswd;
	struct group *grwd;
	int result = 0;

	if (!getegid() || !getgid())
	{
		grwd = getgrnam(safe_group);
		if (!grwd)
		{
			if (verbose)
			{
				printf(_("Security warning: Unable to get GID of group called: %s\n"), safe_group);
				sleep(1);
			}
		}
		else
		{
			if (!getgid() && !getuid())
				result = setgid(grwd->gr_gid);
			else
				result = setegid(grwd->gr_gid);
		}
	}

	if (result==0 && (!geteuid() || !getuid()) )
	{
		pswd = getpwnam(safe_user);
		if (!pswd)
		{
			if (verbose)
			{
				printf(_("Security warning: Unable to get UID of user called: %s\n"), safe_user);
				sleep(1);
			}
		}
		else
		{
			if (!getuid())
				result = setuid(pswd->pw_uid);
			else
				result = seteuid(pswd->pw_uid);
		}
	}

	if (result != 0)
	{
		printf(_("Unable to drop root privileges: %s"), strerror(errno));
		exit(-1);
	}

}
