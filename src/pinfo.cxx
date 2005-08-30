/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
 *  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
 *  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
 *  Copyright 2005  Nathanael Nerode <neroden@gcc.gnu.org>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 ***************************************************************************/

#include "common_includes.h"
#include "signal_handler.h"
#include "utils.h"

#include <string>
using std::string;

RCSID(PKG_VER "$Id$")

#ifdef HAVE_GETOPT_LONG
 #include <getopt.h>
#endif

char *version = VERSION;
int DontHandleWithoutTagTable = 0;

/* currently viewed filename */
string curfile;

/* node specified by --node option */
char *pinfo_start_node = 0;

/* strip `.info' suffix from  "file" */
void strip_info_suffix_from_file(string& file);

/* protect against bad, bad macros */
void checksu();

/* Get options.  Split out of main() to shorten it. */
void
getopts(int argc, char *argv[], string& filename_string, FILE** id) {
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
		{0, 0, 0, 0}
	};

	/******************************************************************************
	 * Parse command line options(getopt)                                         *
	 ******************************************************************************/
	int command_line_option;
	do {
		command_line_option = getopt_long(argc, argv,
			"hvmfrapcsdtnlx", long_options, NULL);
		switch(command_line_option) {
			case 'x':
				ClearScreenAtExit = 1;
				break;
			case 'l':
				LongManualLinks = 1;
				break;
			case 'n':
				if (!optarg) {
					printf(_("--node option used without argument\n"));
					exit(1);
				}
				pinfo_start_node = (char*)malloc(strlen(optarg) + 1);
				strcpy(pinfo_start_node, optarg);
				break;
			/* rcfile */
			case 1:
				if (!optarg) {
					printf(_("--rcfile option used without argument\n"));
					exit(1);
				}
				rcfile = optarg;
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
				exit(0);
			case 'm':
				{
					checksu();
					if (verbose)
						printf(_("Looking for man page...\n"));
					string man_filename_string = "";
					for (int i = optind; i < argc; i++)
					{
						man_filename_string.append(argv[i]);
						man_filename_string.append(" ");
					}
					exit(handlemanual(man_filename_string));
				}
			case 'f':
			case 'r':
				{
					filename_string = argv[argc - 1];
					/* Check for unsafe filenames */
					checkfilename(filename_string);
					/* add the raw path to searchpath */
					addrawpath(filename_string);

					/* Strip suffix in place.  Later, openinfo tries adding all of them */
					strip_compression_suffix(filename_string);
					/* Get basename, and pass to openinfo */
					string basename_string;
					basename(filename_string, basename_string);
					(*id) = openinfo(basename_string, 0);
				}
				break;
			case 'a':
				use_apropos = 1;
				break;
			case 'p':
				{
					use_apropos = 1;
					plain_apropos = 1;
					string man_filename_string = argv[argc - 1];
					exit(handlemanual(man_filename_string));
					/* Again, really really weird.  FIXME. */
				}
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
	} while (command_line_option != EOF);
	/***************************************************************/
#endif
}

int
main(int argc, char *argv[]) {
	int filenotfound = 0;
	WorkRVal work_return_value;
	int userdefinedrc = 0;
	FILE *id = NULL;
	string filename_string;
	/* line count in message */
	long lines = 0;
	/* this will hold node's text */
	char **message = 0;
	/* this will hold the node's header */
	char *type = 0;
	int tag_table_pos = 1;

	/* take care of SIGSEGV, SIGTERM, SIGINT */
	install_signal_handlers();
	searchagain.type = 0;
	searchagain.search = 0;
	initlocale();
	inithistory();
	for (int i = 1; i < argc; i++)
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
		curfile = "dir";
		filename_string = "dir";
	}
	if ((strlen(argv[0]) >= 3)||(use_manual))
		/* handle any 'man' alias to 'pinfo' */
		if ((strstr(argv[0], "man") != NULL)||(use_manual))
		{
			if (verbose)
				printf(_("Looking for man page...\n"));
			string filename_string;
			/*
			 * pass all arguments to the `man' command(manhandler calls
			 * `man')
			 */
			for (int i = 1; i < argc; i++)
			{
				filename_string.append(argv[i]);
				filename_string.append(" ");
			}
			exit(handlemanual(filename_string));
		}

	/* Break out getopts to make main() smaller */
	FILE** idptr = &id;
	getopts(argc, argv, filename_string, idptr);

	checksu();
	initpaths();

	if (argc > 1) {
#ifdef HAVE_GETOPT_LONG
		if (optind < argc)
		{
			/* the paths will be searched by openinfo() */
			filename_string = argv[optind];
		}
		else
		{
			filename_string = "dir";
		}
#else
		/* the paths will be searched by openinfo() */
		filename_string = argv[argc - 1];
#endif

		if ( (filename_string.length() > 0) && (filename_string[0]=='(') )
		{
			string::size_type j = filename_string.find(')');
			if (j != string::npos) {
				/* Looks like filename and node. */
				/* copy the node content to pinfo_start_node */
				if (!pinfo_start_node)
				{
					pinfo_start_node=strdup(filename_string.substr(j+1).c_str());
				}
				/* leave the filename part in filename */
				filename_string.resize(j);
				/* and erase the leading '(' */
				filename_string.erase(0);
			}
		}

		/* security check */
		checkfilename(filename_string);

		/* autodetect raw filenames */
		if (    (    filename_string.length() >= 1
		          && filename_string[0] == '/')
		     || (    filename_string.length() >= 2
		          && filename_string.compare(0, 2, "./") == 0)
		     || (    filename_string.length() >= 3
		          && filename_string.compare(0, 3, "../") == 0)
		   )
		{
			addrawpath(filename_string);
		}
		curfile = filename_string;
	}

	/* no rawpath has been opened */
	if (id == NULL) {
		id = openinfo(filename_string, 0);
	}

	/* try to lookup the name in dir file */
	if (id == NULL)
	{
		id = dirpage_lookup(&type, &message, &lines, filename_string,
												&pinfo_start_node);
	}

	/* if still nothing, try to use man page instead */
	if (id == NULL)
	{
		printf(_("Error: could not open info file, trying manual\n"));
		exit(handlemanual(filename_string));
	}

	/* search for indirect entries, if any */
	if (seek_indirect(id))
	{
		read_item(id, &type, &message, &lines);
		load_indirect(message, lines);
	}

	/* load tag table if such exists... */
	if (seek_tag_table(id,1) != 2) {
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
	} else { /* ...otherwise try to create one */
		if ( verbose && (curfile != "dir") )
			printf(_("Warning: tag table not found...\n"));
		if (!DontHandleWithoutTagTable)
		{
			if ( verbose && (curfile != "dir") )
				printf(_("Trying to create alternate tag table...\n"));
			create_tag_table(id);
			/* if there weren't found any info entries */
			if (TagTableEntries < 1)
			{
				printf(_("This doesn't look like an info file...\n"));
				exit(handlemanual(filename_string));
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
		seeknode(tag_table_pos, &id);
		/* read the node */
		read_item(id, &type, &message, &lines);

		/* handle goto/link where no file was found -- see bellow */
		if (!filenotfound)
			addinfohistory(curfile.c_str(), tag_table[tag_table_pos].nodename, -1, -1, -1);
		else
			filenotfound = 0;
		work_return_value = work(&message, &type, &lines, id, tag_table_pos);
		if (work_return_value.keep_going)
		{
			/* no cross-file link selected */
			if (work_return_value.file[0] == 0)
			{
				int tmppos = gettagtablepos(work_return_value.node.c_str());
				if (tmppos != -1)
					tag_table_pos = tmppos;
			}
			else /* file was specified */
			{
				strip_info_suffix_from_file(work_return_value.file);
				/* file name was the same with the file currently viewed */
				if (curfile == work_return_value.file)
				{
					int tmppos = gettagtablepos(work_return_value.node.c_str());
					if (tmppos != -1)
						tag_table_pos = tmppos;
				}
				else /* open new info file */
				{
					fclose(id);
					/* Reset global filenameprefix */
					filenameprefix.clear();
					id = openinfo(work_return_value.file, 0);

					char *tmp = NULL;
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
						strip_info_suffix_from_file(curfile);
						string tmpstr = curfile;
						id = openinfo(tmpstr, 0);
						tmp = NULL;
						if (id == NULL)
						{
							closeprogram();
							printf(_("Unexpected error.\n"));
							return 1;
						}
					}
					else /* if we succeeded in opening new file */
					{
						curfile = work_return_value.file;
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
						if (work_return_value.node != "")
						{
							int tmptagtablepos = gettagtablepos(work_return_value.node.c_str()	);
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
	while (work_return_value.keep_going);
	fclose(id);
	closeprogram();
	/* free's at the end are optional, but look nice :) */
	freeitem(&type, &message, &lines);
	freetagtable();
	freeindirect();
	return 0;
}

void
strip_info_suffix_from_file(string& file)
{
	if ( (file.length() > 5) &&
	     (file.compare(file.length() - 5, 5, ".info") == 0)
		 ) {
		file.resize(file.length() - 5);
	}
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

	if (!getegid() || !getgid())
	{
		grwd = getgrnam(safe_group.c_str());
		if (!grwd)
		{
			if (verbose)
			{
				printf(_("Security warning: Unable to get GID of group called: %s\n"), safe_group.c_str());
				sleep(1);
			}
		}
		else
		{
			if (!getgid() && !getuid())
				setgid(grwd->gr_gid);
			else
				setegid(grwd->gr_gid);
		}
	}

	if (!geteuid() || !getuid())
	{
		pswd = getpwnam(safe_user.c_str());
		if (!pswd)
		{
			if (verbose)
			{
				printf(_("Security warning: Unable to get UID of user called: %s\n"), safe_user.c_str());
				sleep(1);
			}
		}
		else
		{
			if (!getuid())
				setuid(pswd->pw_uid);
			else
				seteuid(pswd->pw_uid);
		}
	}

}
