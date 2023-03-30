/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
 *  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
 *  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
 *  Copyright (C) 2005  Nathanael Nerode <neroden@gcc.gnu.org>
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

typedef struct
{
	char *suffix;
	char *command;
}
Suffixes;


/******************************************************************************
 * This piece of declarations says what to do with info files stored with      *
 * different formats/compression methods, before putting them into a temporary *
 * file. I.e. you don't do anything to plain `.info' suffix; for a `.info.gz'  *
 * you dump the file through `gunzip -d -c', etc.                              *
 ******************************************************************************/

#define SuffixesNumber 4

Suffixes suffixes[SuffixesNumber] =
{
	{"", 		"cat"},
	{".gz",		"gzip -d -q -c"},
	{".Z",		"gzip -d -q -c"},
	{".bz2",	"bzip2 -d -c"}
};

/*****************************************************************************/

char **infopaths = 0;
int infopathcount = 0;

int
qsort_cmp(const void *base, const void *compared)
{
	char *cbase =((TagTable *) base)->nodename;
	char *ccompared =((TagTable *) compared)->nodename;
	return compare_tag_table_string(cbase, ccompared);
}

int
matchfile(char **buf, char *name)
{
#define Buf	(*buf)
	DIR *dir;
	char *bname = basename(name);
	struct dirent *dp;
	int matched = 0;

	/* remove a possible ".info" from the end of the file name
	 * we're looking for */
	strip_info_suffix(bname);

	/* fix the name of the dir */
	if (Buf[strlen(Buf)-1]!='/')
	{
		strcat(Buf,"/");
	}
	strncat(Buf,name,bname-name);

	/* open the directory */
	dir = opendir(Buf);
	if (dir == NULL)
	{
		return 0;
	}

	/* iterate over all files in the directory */
	while ((dp = readdir(dir)) != NULL)
	{
		/* use strcat rather than strdup, because xmalloc handles all
		 * malloc errors */
		char *filename = xmalloc(strlen(dp->d_name)+1);
		char *pagename = xmalloc(strlen(dp->d_name)+1);
		strcat(filename, dp->d_name);
		strcat(pagename, dp->d_name);

		/* strip suffixes (so "gcc.info.gz" -> "gcc") */
		strip_compression_suffix(pagename);
		strip_info_suffix(pagename);

		/* strip compression suffix from returned filename
		 * decompresison and type matching will happen later
		 * (sigh)
		 */
		strip_compression_suffix(filename);

		//fprintf(stdout,"Found filename `%s' (%s)\n", filename, pagename);

		/* compare this file with the file we're looking for */
		if (strcmp(pagename,bname) == 0)
		{
			/* we found a match! */
			matched++;
			/* put it in the buffer */
			strncat(Buf, filename, 1023-strlen(Buf));

			/* clean up, and exit the loop */
			xfree(filename);
			xfree(pagename);
			break;
		}
		xfree(filename);
		xfree(pagename);
	}
	closedir(dir);

	if (matched) return 1;

	return 0;
#undef Buf
}

FILE *
dirpage_lookup(char **type, char ***message, unsigned long *lines,
		char *filename, char **first_node)
{
#define Type	(*type)
#define Message	(*message)
#define Lines	(*lines)
	FILE *id = 0;
	int filenamelen = strlen(filename);
	int goodHit = 0;
	char name[256];
	char file[256];
	char *nameend, *filestart, *fileend, *dot;

	id = opendirfile(0);
	if (!id)
		return NULL;

	read_item(id, type, message, lines);

	/* search for node-links in every line */
	for (unsigned long i = 1; i < Lines; i++)
	{
		if ( (Message[i][0] == '*') && (Message[i][1] == ' ')
				&& ( nameend = strchr(Message[i], ':') )
				&& (*(nameend + 1) != ':')	/* form: `* name:(file)node.' */
				&& (filestart = strchr(nameend, '(') )
				&& (fileend = strchr(filestart, ')') )
				&& (dot = strchr(fileend, '.') )
				&& (strncasecmp(filename, Message[i] + 2, filenamelen) == 0)
		   )
		{
			char *tmp;

			/* skip this hit if it is not a perfect match and
			 * we have already found a previous partial match */
			if ( ! ( (nameend - Message[i]) - 2 == filenamelen )
					&&	goodHit )
			{
				continue;
			}

			/* find the name of the node link */
			tmp = name;
			strncpy(file, filestart + 1, fileend - filestart - 1);
			file[fileend - filestart - 1] = 0;
			strncpy(name, fileend + 1, dot - fileend - 1);
			name[dot - fileend - 1] = 0;
			while (isspace(*tmp)) tmp++;

			if (strlen(name))
			{
				*first_node = xmalloc(strlen(tmp) + 1);
				strcpy((*first_node), tmp);
			}

			/* close the previously opened file */
			if (id)
			{
				fclose(id);
				id = 0;
			}

			/* see if this info file exists */
			id = openinfo(file, 0);
			if (id)
			{
				goodHit = 1;
			}
		}
	}

	/* if we haven't found anything, clean up and exit */
	if (id && !goodHit)
	{
		fclose(id);
		id = 0;
	}

	/* return file we found */
	return id;
#undef Lines
#undef Message
#undef Type
}

void
freeitem(char **type, char ***buf, unsigned long *lines)
{
#define Type	(*type)
#define Buf		(*buf)
#define Lines	(*lines)

	if (Type != 0)
	{
		xfree(Type);
		Type = 0;
	}
	if (Buf != 0)
	{
		for (unsigned long i = 1; i <= Lines; i++)
			if (Buf[i] != 0)
			{
				xfree(Buf[i]);
				Buf[i] = 0;
			}
		xfree(Buf);
		Buf = 0;
	}
#undef Type
#undef Buf
#undef Lines
}

void
read_item(FILE * id, char **type, char ***buf, unsigned long *lines)
{

#define Type	(*type)
#define Buf		(*buf)
#define Lines	(*lines)
	int i;

	freeitem(type, buf, lines);	/* free previously allocated memory */

	/* set number of lines to 0 */
	Lines = 0;

	/* initial buffer allocation */
	Buf = xmalloc(sizeof(char **));

	/* seek precisely on the INFO_TAG (the seeknode function may be imprecise
	 * in combination with some weird tag_tables).  */
	while (!feof(id) && fgetc(id) != INFO_TAG);
	/* then skip the trailing `\n' */
	while (!feof(id) && fgetc(id) != '\n');

	/* allocate and read the header line */
	Type = xmalloc(1024);
	if (fgets(Type, 1024, id)==NULL)
	{
		/* nothing to do */
		return;
	}
	Type = xrealloc(Type, strlen(Type) + 1);
	/* now iterate over the lines */
	do
	{
		/* don't read after eof in info file */
		if (feof(id))
			break;

		/* realloc the previous line for it to fit exactly */
		if (Lines)
		{
			Buf[Lines] = xrealloc(Buf[Lines], strlen(Buf[Lines]) + 1);
		}

		/* TODO: Weirdness going on here; looks like off-by-one error as Buf[0] is always "\0" */
		/* increase the read lines number */
		Lines++;

		/* allocate space for the new line */
		Buf = xrealloc(Buf, sizeof(char **) *(Lines + 1));
		Buf[Lines] = xmalloc(1024);
		Buf[Lines][0] = 0;

		/* if the line was not found in input file, fill the allocated space
		 * with empty line.  */
		if (fgets(Buf[Lines], 1024, id) == NULL)
			strcpy(Buf[Lines], "\n");
		else /* we can be sure that at least 1 char was read! */
		{
			/* *sigh*  indices contains \0's
			 * which totally fucks up all strlen()s.
			 * so replace it by a space */
			i = 1023;
			/* find the end of the string */
			while (Buf[Lines][i]=='\0' && i>=0) i--;
			/* and replace all \0's in the rest of the string by spaces */
			while (i>=0)
			{
				if (Buf[Lines][i]=='\0' || Buf[Lines][i]=='\b')
					Buf[Lines][i]=' ';
				i--;
			}
		}
	}
	while (Buf[Lines][0] != INFO_TAG);	/* repeat until new node mark is found */


	/* added for simplifing two-line ismenu and isnote functs */
	if (Lines)
	{
		strcpy(Buf[Lines], "\n");
		Buf[Lines] = xrealloc(Buf[Lines], strlen(Buf[Lines]) + 1);
	}

	fseek(id, -2, SEEK_CUR);

#undef Type
#undef Buf
#undef Lines

}
void
load_indirect(char **message, unsigned long lines)
{
	char *wsk;
	int cut = 0;			/* number of invalid entries */
	indirect = xmalloc((lines + 1) * sizeof(Indirect));
	for (unsigned long i = 1; i < lines; i++)
	{
		char *check;
		wsk = message[i];
		check = wsk + strlen(wsk);
		while (*(++wsk) != ':')	/* check if this line keeps a real entry */
		{
			if (wsk == check)	/*
								 * make sure wsk won't go out of range
								 * in case the wsk would be corrupted.
								 */
				break;
		}
		if (*wsk)			/* if the entry holds some data... */
		{
			(*wsk) = 0;
			strncpy(indirect[i - cut].filename, message[i], 200);
			(*wsk) = ':';
			indirect[i - cut].offset = atoi(wsk + 2);
		}
		else
			cut++;			/* if the entry was invalid, make inirect count shorter */
	}
	IndirectEntries = lines - 1 - cut;
}

void
load_tag_table(char **message, unsigned long lines)
{
	char *wsk, *wsk1;
	int is_indirect = 0;
	register unsigned int j;
	register char *res;
	int cut = 0;			/* holds the number of corrupt lines */

	/*
	 * if in the first line there is a(indirect) string, skip that line
	 * by adding the value of is_indirect=1 to all message[line] references.
	 */
	if (strcasecmp("(Indirect)", message[1]) == 0)
		is_indirect = 1;
	tag_table = xmalloc((lines + 1) * sizeof(TagTable));
	for (unsigned long i = 1; i < lines - is_indirect; i++)
	{
		char *check;
		wsk = message[i + is_indirect];
		check = wsk + strlen(wsk);
		while (!isspace(*(++wsk)))
		{
			if (wsk >= check)
			{
				wsk--;
				break;
			}
		}
		wsk++;
		wsk1 = wsk;
		check = wsk1 + strlen(wsk1);
		while (*(++wsk1) != INDIRECT_TAG)
		{
			if (wsk1 >= check)
				break;
		}
		if (wsk1 < check)
		{
			(*wsk1) = 0;
			/*
			 * original: sprintf(tag_table[i-cut].nodename,"%s",wsk);
			 * below is a faster version.
			 */
			res = memcpy(tag_table[i - cut].nodename, wsk, j =(size_t)(wsk1 - wsk));
			(*(res += j + 1)) = 0;
			(*wsk1) = INDIRECT_TAG;
			wsk1++;
			tag_table[i - cut].offset = atoi(wsk1);
		}
		else
			cut++;			/* increment the number of corrupt entries */
	}
	TagTableEntries = lines - 1 - is_indirect - cut;

	/* FIXME: info should ALWAYS start at the 'Top' node, not at the first
	   mentioned node(vide ocaml.info) */

	for (unsigned int i = 1; i <= TagTableEntries; i++)
	{
		if (strcasecmp(tag_table[i].nodename, "Top") == 0)
		{
			FirstNodeOffset = tag_table[i].offset;
			strcpy(FirstNodeName, tag_table[i].nodename);
		}
	}
	qsort(&tag_table[1], TagTableEntries, sizeof(TagTable), qsort_cmp);
}

/* TODO: seek_indirect() and seek_tag_table() are almost identical: remove duplicate code */
int
seek_indirect(FILE * id)
{
	int finito = 0;
	long seek_pos;
	int input;
	char *type = xmalloc(1024);
	fseek(id, 0, SEEK_SET);
	while (!finito)		/*
						 * scan through the file, searching for "indirect:"
						 * string in the type(header) line of node.
						 */
	{
		while ((input = fgetc(id)) != INFO_TAG)
			if (input == EOF)
			{
				if (type)
				{
					xfree(type);
					type = 0;
				}
				return 0;
			}
		seek_pos = ftell(id) - 2;
		fgetc(id);
		if (fgets(type, 1024, id)==0)
		{
			/* we're at the end of the file and haven't found any indirect refs. so bail out */
			if (type)
			{
				xfree(type);
				type = 0;
			}
			return 0;
		}
		if (strncasecmp("Indirect:", type, strlen("Indirect:")) == 0)
		{
			finito = 1;
		}
	}
	xfree(type);
	type = 0;
	if (!curses_open)
	{
		if (verbose)
			printf(_("Searching for indirect done"));
		printf("\n");
	}
	else
	{
		attrset(bottomline);
		mvhline(maxy - 1, 0, ' ', maxx);
		if (verbose)
			mvaddstr(maxy - 1, 0, _("Searching for indirect done"));
		attrset(normal);
	}
	fseek(id, seek_pos, SEEK_SET);
	return 1;
}

/*
 * second arg for dumping out verbose debug info or not :)
 */
int
seek_tag_table(FILE * id,int quiet)
{
	int finito = 0;
	long seek_pos;
	int input;
	char *type = xmalloc(1024);
	fseek(id, 0, SEEK_SET);
	/*
	 * Scan through the file, searching for a string
	 * "Tag Table:" in the type(header) line of node.
	 */
	while (!finito)
	{
		while ((input = fgetc(id)) != INFO_TAG)
		{
			if (input == EOF)
			{
				if (!quiet)
				{
					if (!curses_open)
					{
						printf(_("Warning: could not find tag table"));
						printf("\n");
					}
					else
					{
						attrset(bottomline);
						mvhline(maxy - 1, 0, ' ', maxx);
						mvaddstr(maxy - 1, 0, _("Warning: could not find tag table"));
						attrset(normal);
					}
				}
				if (type)
				{
					xfree(type);
					type = 0;
				}
				return 2;
			}
		}
		seek_pos = ftell(id) - 2;
		while (fgetc(id) != '\n');
		if (fgets(type, 1024, id)==NULL)
		{
			/* we're at the end of the file and haven't found a tag table. so bail out */
			if (type)
			{
				xfree(type);
				type = 0;
			}
			return 2;
		}
		if (strncasecmp("Tag Table:", type, strlen("Tag Table:")) == 0)
		{
			finito = 1;
		}
	}
	xfree(type);
	type = 0;
	if (!curses_open)
	{
		if (verbose)
			printf(_("Searching for tag table done\n"));
	}
	else
	{
		attrset(bottomline);
		mvhline(maxy - 1, 0, ' ', maxx);
		if (verbose)
			mvaddstr(maxy - 1, 0, "Searching for tag table done");
		attrset(normal);
	}
	fseek(id, seek_pos, SEEK_SET);
	return 1;
}

void
buildcommand(char *dest, char *command, char *filename, const char *tmpfilename)
{
	strcpy(dest, command);
	strcat(dest, " ");
	strcat(dest, filename);
	strcat(dest, "> ");
	strcat(dest, tmpfilename);
}

void
builddircommand(char *dest, char *command, char *filename, const char *tmpfilename)
{
	strcpy(dest, command);
	strcat(dest, " ");
	strcat(dest, filename);
	strcat(dest, ">> ");
	strcat(dest, tmpfilename);
}

FILE *
opendirfile(int number)
{
	FILE *id = NULL;
	char buf[1024];		/* holds local copy of filename */
	char *bufend;			/* points at the trailing 0 of initial name */
	char command[1128];		/* holds command to evaluate for decompression of file */
	int i, j;
	char *tmpfilename = NULL;
	size_t *fileendentries = xmalloc(infopathcount * sizeof(*fileendentries)); /* should really be off_t, but a signed type really doesn't make sense here */
	int dir_found = 0;
	int dircount = 0;
	int lang_found;
	struct stat status;

	if (number == 0)		/* initialize tmp filename for file 1 */
	{
		/* close and delete old tmp file */
		if (tmpfilename1)
		{
			unlink(tmpfilename1);	/* erase old tmpfile */
			free(tmpfilename1);
		}
		tmpfilename1 = make_tempfile();
		tmpfilename = tmpfilename1;	/* later we will refere only to tmp1 */
	}
	for (i = 0; i < infopathcount; i++)	/* go through all paths */
	{
		lang_found = 0;
		strcpy(buf, infopaths[i]);	/* build a filename */
		strcat(buf, "/");
		if (getenv("LANG") != NULL)
			strcat(buf, getenv("LANG"));
		strcat(buf, "/dir");
		/*
		 * remember the bufend to make it
		 * possible later to glue compression suffixes.
		 */
		bufend = buf;
		bufend += strlen(buf);
		for (j = 0; j < SuffixesNumber; j++)	/* go through all suffixes */
		{
			strcat(buf, suffixes[j].suffix);
			if ((id = fopen(buf, "r")) != NULL)
			{
				fclose(id);
				builddircommand(command, suffixes[j].command, buf, tmpfilename);
				xsystem(command);
				lstat(tmpfilename, &status);
				fileendentries[dircount] = status.st_size;
				dircount++;
				dir_found = 1;
				lang_found = 1;
			}
			(*bufend) = 0;
		}

		/* same as above, but without $LANG support */
		if (!lang_found)
		{
			strcpy(buf, infopaths[i]);	/* build a filename */
			strcat(buf, "/");
			strcat(buf, "dir");
			/*
			 * remember the bufend to make it possible later to glue
			 * compression suffixes.
			 */
			bufend = buf;
			bufend += strlen(buf);
			for (j = 0; j < SuffixesNumber; j++)	/* go through all suffixes */
			{
				strcat(buf, suffixes[j].suffix);
				if ((id = fopen(buf, "r")) != NULL)
				{
					fclose(id);
					builddircommand(command, suffixes[j].command, buf, tmpfilename);
					xsystem(command);
					lstat(tmpfilename, &status);
					fileendentries[dircount] = status.st_size;
					dircount++;
					dir_found = 1;
				}
				(*bufend) = 0;
			}
		}
	}
	if (dir_found)
		id = fopen(tmpfilename, "r");
	/*
	 * Filter the concatenated dir pages to exclude hidden parts of info
	 * entries
	 */
	if (id)
	{
		char *tmp;
		size_t filelen, l;
		int aswitch = 0;
		int firstswitch = 0;
		dircount = 0;

		fseek(id, 0, SEEK_END);
		filelen = ftell(id);

		tmp = xmalloc(filelen);
		fseek(id, 0, SEEK_SET);
		if (fread(tmp, 1, filelen, id)!=filelen)
		{
			printf(_("Error while reading file '%s'"), tmp);
			closeprogram();
			exit(1);
		}
		fclose(id);
		id = fopen(tmpfilename, "w");
		for (l = 0; l < filelen; l++)
		{
			if (tmp[l] == INFO_TAG)
			{
				aswitch ^= 1;
				if (!firstswitch)
					fputc(tmp[l], id);
				firstswitch = 1;
			}
			else if ((aswitch) ||(!firstswitch))
				fputc(tmp[l], id);
			if (l + 1 == fileendentries[dircount])
			{
				if (aswitch != 0)
					aswitch = 0;
				dircount++;	/* the last dircount should fit to the end of filelen */
			}
		}
		fputc(INFO_TAG, id);
		fputc('\n', id);
		xfree(fileendentries);
		fclose(id);
		id = fopen(tmpfilename, "r");
		xfree(tmp);

		return id;
	}
	return NULL;
}

/*
 * Note: openinfo is a function for reading info files, and putting
 * uncompressed content into a temporary filename.  For a flexibility, there
 * are two temporary files supported, i.e.  one for keeping opened info file,
 * and second for i.e. regexp search across info nodes, which are in other
 * info-subfiles.  The temporary file 1 is refrenced by number=0, and file 2 by
 * number=1 Openinfo by default first tries the path stored in char
 * *filenameprefix and then in the rest of userdefined paths.
 */
FILE *
openinfo(char *filename, int number)
{
	FILE *id = NULL;
#define BUF_LEN 1024
	char *buf = xmalloc(BUF_LEN);	/* holds local copy of filename */
	char *bufend;			/* points at the trailing 0 of initial name */
	char command[1128];		/* holds command to evaluate for decompression of file */
	int i, j;
	char *tmpfilename;

	if ((strncmp(filename, "dir", 3)==0)  &&  !isalnum(filename[3]))
	{
		xfree(buf);
		return opendirfile(number);
	}

	if (number == 0)		/* initialize tmp filename for file 1 */
	{
		if (tmpfilename1)
		{
			unlink(tmpfilename1);	/* erase old tmpfile */
			free(tmpfilename1);
		}
		tmpfilename1 = make_tempfile();
		tmpfilename = tmpfilename1;	/* later we will refere only to tmp1 */
	}
	else /* initialize tmp filename for file 2 */
	{
		if (tmpfilename2)
		{
			unlink(tmpfilename2);	/* erase old tmpfile */
			free(tmpfilename2);
		}
		tmpfilename2 = make_tempfile();
		tmpfilename = tmpfilename2;	/* later we will refere only to tmp2 */
	}

	for (i = -2; i < infopathcount; i++)	/* go through all paths */
	{
		if (i < 0)
		{
			/*
			 * no filenameprefix, we don't navigate around any specific
			 * infopage set, so simply scan all directories for a hit
			 */
			if (!filenameprefix)
				continue;
			/* build a filename: First (i == -2) try filenameprefix/filename,
			 * then try with a .info appended */
			if (i == -2)
				snprintf(buf, BUF_LEN, "%s/%s", filenameprefix, basename(filename));
			else
				snprintf(buf, BUF_LEN, "%s/%s.info", filenameprefix, basename(filename));
		}
		else
		{
			/* build a filename */
			strcpy(buf, infopaths[i]);
			/* no match found in this directory */
			if (! matchfile(&buf, filename))
				continue;
		}
		bufend = buf;
		/* remember the bufend to make it possible later to glue compression
		 * suffixes. */
		bufend += strlen(buf);
		for (j = 0; j < SuffixesNumber; j++)	/* go through all suffixes */
		{
			strcat(buf, suffixes[j].suffix);
			if ((id = fopen(buf, "r")) != NULL)
			{
				fclose(id);
				clearfilenameprefix();
				filenameprefix = strdup(buf);
				{			/* small scope for removal of filename */
					int prefixi, prefixlen = strlen(filenameprefix);
					for (prefixi = prefixlen; prefixi > 0; prefixi--)
						if (filenameprefix[prefixi] == '/')
						{
							filenameprefix[prefixi] = 0;
							break;
						}
				}
				buildcommand(command, suffixes[j].command, buf, tmpfilename);
				xsystem(command);
				id = fopen(tmpfilename, "r");
				if (id)
				{
					xfree(buf);
					return id;
				}
			}
			(*bufend) = 0;
		}

		/* if we have a nonzero filename prefix, that is we view a set of
		 * infopages, we don't want to search for a page in all
		 * directories, but only in the prefix directory.  Therefore break
		 * here. */
		if ((i == -1) &&(filenameprefix))
			break;
	}
	xfree(buf);


	return 0;
#undef BUF_LEN
}

	void
addrawpath(char *filename)
{
	int len = strlen(filename);
	int i, pos;
	char tmp = '\0';
	for (i = len; i >= 0; i--)
	{
		if (filename[i] == '/')
		{
			tmp = filename[i+1];
			filename[i+1] = 0;
			pos = i+1;
			break;
		}
	}
	if (i < 0)
		pos = -1;

	infopaths = xrealloc(infopaths,(infopathcount + 3) *(sizeof(char *)));
	for (i = infopathcount; i > 0; i--)	/* move entries to the right */
		infopaths[i] = infopaths[i - 1];

	if (pos > 0)
		infopaths[0]=strdup(filename);	/* add new(raw) entry */
	else
		infopaths[0]=strdup("./");
	infopathcount++;

	if (pos > 0)			/* recreate original filename */
		filename[pos] = tmp;
}

	int
isininfopath(char *name)
{
	int i;
	for (i = 0; i < infopathcount; i++)
	{
		if (strcmp(name, infopaths[i]) == 0)
			return 1;		/* path already exists */
	}
	return 0;			/* path not found in previous links */
}

/* returns the number of chars ch in string str */
unsigned int
charcount(const char *str, const char ch)
{
	int num = 0;
	const char *c;

	c = str;

	while (*c != '\0')
	{
		if (*c++ == ch)
			num++;
	}
	return num;
}

/*
 * find the paths where info files are to be found,
 * and put them in the global var infopaths[]
 */
void
initpaths()
{
	char emptystr[1] = "";
	char **paths = NULL;
	char *infopath = NULL, *langpath = NULL;
	char *c, *dir, *env, *next;
	char *rawlang = NULL, *lang = NULL, *langshort = NULL;
	int ret;
	unsigned int i, j, maxpaths, numpaths = 0, infolen, langlen;
	size_t len;
	struct stat sbuf;
	ino_t *inodes;

	/* first concat the paths */
	env = getenv("INFOPATH");
	if (env == NULL)
	{
		env = emptystr;
	}
	infolen = strlen(env) + strlen(configuredinfopath) + 3;
	infopath = (char *) xmalloc( infolen );
	strcat(infopath, env);
	strcat(infopath, ":");
	strcat(infopath, configuredinfopath);
	/* end with a :, otherwise the strchr below will fail for the last entry */
	strcat(infopath, ":");

	/* alloc the paths[] array */
	maxpaths = 3 * (charcount( infopath, ':' ) + 1); // *3 for $LANG
	paths = (char **) xmalloc( maxpaths * sizeof(char *) );

	/* split at ':' and put the path components into paths[] */
	dir = infopath;
	/* if this actually is a non-empty string, add it to paths[] */
	while ( (next = strchr(dir, ':')) != NULL )
	{
		*next = '\0';  /* terminate the string */

		/* if the dir actually is a non-empty string, add it to paths[] */
		if ( dir && strlen(dir)>0 )
		{
			paths[numpaths++] = dir;
		}

		/* and advance the pointer to the next entry */
		dir = next+1;
	}

	/* get the current $LANG, if any (to use for localized info pages) */
	rawlang = getenv("LANG");
	if (rawlang) {
		lang = strdup(rawlang);
		/* fix the lang string */
		for (i=0; lang[i]!='\0'; i++)
		{
			/* cut off the charset */
			if (lang[i]=='.')
			{
				lang[i]='\0';
			}
			/* if lang is sublocalized (nl_BE or so), also use short version */
			if (lang[i]=='_' && langshort==NULL)
			{
				langshort = strdup(lang);
				langshort[i] = '\0';
			}
		}
	}
	/* if we have a LANG defined, add paths with this lang to the paths[] */
	if (lang && strlen(lang)>0 )
	{
		/* crude upper limit */
		langlen = infolen + (strlen(lang)+2) * numpaths + 1;
		if (langshort!=NULL) langlen *= 2;
		langpath = (char *) xmalloc( langlen * sizeof(char) );

		c = langpath;
		for (i=0; i<numpaths; i++)
		{
			/* TODO: check for negative return values of sprintf */
			len = sprintf(c, "%s/%s", paths[i], lang);
			/* add the lang specific dir at the beginning */
			paths[numpaths+i] = paths[i];
			paths[i] = c;

			c += len+1;

			if (langshort)
			{
				/* TODO: check for negative return values of sprintf */
				len = sprintf(c, "%s/%s", paths[numpaths+i], langshort);
				/* add the lang specific dir at the beginning */
				paths[2*numpaths+i] = paths[numpaths+i];
				paths[numpaths+i] = c;

				c += len+1;
			}

		}
		numpaths *= (langshort?3:2);
	}

#ifdef ___DEBUG___
	/* for debugging */
	for (i=0; i<numpaths; i++)
		fprintf(stderr,"--> %s\n", paths[i]);
#endif

	/* ok, now we have all the (possibly) revelevant paths in paths[] */
	/* now loop over them, see if they are valid and if they are duplicates*/
	/* TODO: cleanup all malloc calls (get rid of cast, use sizeof(varname) instead of sizeof(type) */
	inodes = (ino_t *) xmalloc( maxpaths * sizeof(ino_t) );
	numpaths = 0;
	len = 0;
	for (i=0; i< maxpaths; i++)
	{
		/* TODO: check where these NULL paths come from */
		if (paths[i]==NULL)
			continue;

		/* stat() the dir */
		ret = stat( paths[i], &sbuf);
		/* and see if it could be opened */
		if (ret < 0)
		{
#ifdef ___DEBUG___
			fprintf(stderr, "error while opening `%s': %s\n",
					paths[i], strerror(errno) );
#endif
			paths[i] = NULL;
			inodes[i] = 0;
		}
		else
		{
			inodes[i] = sbuf.st_ino;
		}

		/* now check if this path is a duplicate */
		for (j=0; j<i; j++)
		{
			if (inodes[j]==inodes[i]) paths[i] = NULL;
		}

		/* calculate the total number of vali paths and the size of teh strings */
		if (paths[i]!=NULL)
		{
			numpaths++;
			len += strlen(paths[i]) + 1;
		}
	}


	/* and alloc and copy to global var */
	infopathcount = numpaths;
	infopaths = (char **) xmalloc( numpaths * sizeof(char *) );
	c = (char *) xmalloc( len * sizeof(char) );
	j=0;
	for (i=0; i<maxpaths; i++)
	{
		if (paths[i]!=NULL)
		{
			/* copy path to c buffer */
			strcpy(c, paths[i]);
			infopaths[j++] = c;
			c += strlen(paths[i]) + 1;
		}
	}


	xfree(infopath);
	xfree(langpath);
	xfree(paths);
	xfree(lang);
	xfree(langshort);
	xfree(inodes);

#ifdef ___DEBUG___
	/* for debugging */
	fprintf(stderr, "%i valid info paths found:\n", infopathcount);
	for (i=0; i<infopathcount; i++)
		if (infopaths[i]) fprintf(stderr,"--> %s\n", infopaths[i]);
#endif


}



void
create_indirect_tag_table()
{
	FILE *id = 0;
	int initial;
	for (unsigned i = 1; i <= IndirectEntries; i++)
	{
		id = openinfo(indirect[i].filename, 1);
		initial = TagTableEntries + 1;
		if (!id)
		{
			/* display error message to make the user aware of
			 * the broken info page
			 */
			char msg[1024];
			snprintf(msg, 1024, "%s '%s' (%s)",
				_("Can't open file"), indirect[i].filename,
				_("press a key to continue") );
			attrset(bottomline);
			mvhline(maxy - 1, 0, ' ', maxx);
			mvaddstr(maxy - 1, 0, msg);
			move(0, 0);
			attrset(normal);
			getch();

			continue;
		}
		create_tag_table(id);
		FirstNodeOffset = tag_table[1].offset;
		strcpy(FirstNodeName, tag_table[1].nodename);
		fclose(id);
		for (unsigned j = initial; j <= TagTableEntries; j++)
		{
			tag_table[j].offset +=(indirect[i].offset - FirstNodeOffset);
		}
	}
	FirstNodeOffset = tag_table[1].offset;
	strcpy(FirstNodeName, tag_table[1].nodename);
	qsort(&tag_table[1], TagTableEntries, sizeof(TagTable), qsort_cmp);
}
void
create_tag_table(FILE * id)
{
	char *buf = xmalloc(1024);
	long oldpos;
	fseek(id, 0, SEEK_SET);
	if (!tag_table)
		tag_table = xmalloc((TagTableEntries + 2) * sizeof(TagTable));
	else
		tag_table = xrealloc(tag_table,(TagTableEntries + 2) * sizeof(TagTable));
	while (!feof(id))
	{
		if (fgetc(id) == INFO_TAG)	/* We've found a node entry! */
		{
			while (fgetc(id) != '\n');	/* skip '\n' */
			TagTableEntries++;	/* increase the nuber of tag table entries */
			oldpos = ftell(id);	/* remember this file position! */
			/*
			 * it is a an eof-fake-node (in some info files it happens, that
			 * the eof'ish end of node is additionaly signalised by an INFO_TAG
			 * We give to such node an unlike to meet nodename.
			 */
			if (fgets(buf, 1024, id) == NULL)
			{
				tag_table = xrealloc(tag_table, sizeof(TagTable) *(TagTableEntries + 1));
				strcpy(tag_table[TagTableEntries].nodename, "12#!@#4");
				tag_table[TagTableEntries].offset = 0;
			}
			else
			{
				int colons = 0, i, j;
				int buflen = strlen(buf);
				for (i = 0; i < buflen; i++)
				{
					if (buf[i] == ':')
						colons++;
					if (colons == 2)	/*
										 * the string after the second colon
										 * holds the name of current node.
										 * The name may then end with `.',
										 * or with a newline, which is scanned
										 * bellow.
										 */
					{
						for (j = i + 2; j < buflen; j++)
						{
							if ((buf[j] == ',') ||(buf[j] == '\n'))
							{
								tag_table = xrealloc(tag_table, sizeof(TagTable) *(TagTableEntries + 1));
								buf[j] = 0;
								buflen = j;
								strcpy(tag_table[TagTableEntries].nodename, buf + i + 2);
								tag_table[TagTableEntries].offset = oldpos - 2;
								break;
							}
						}
						break;
					}
				}		/* end: for loop, looking for second colon */
			}			/* end: not a fake node */
		}			/* end: we've found a node entry(INFO_TAG) */
	}				/* end: global while loop, looping until eof */
	xfree(buf);
	buf = 0;
	if (!indirect)
	{
		FirstNodeOffset = tag_table[1].offset;
		strcpy(FirstNodeName, tag_table[1].nodename);
		qsort(&tag_table[1], TagTableEntries, sizeof(TagTable), qsort_cmp);
	}
}

int
seeknode(int tag_table_pos, FILE ** Id)
{
	int i;
	FILE * newid;
#define id	(*Id)
	/*
	 * Indirect nodes are seeked using a formula:
	 * file-offset = tagtable_offset - indirect_offset +
	 *             + tagtable[1]_offset
	 */
	if (indirect)
	{
		for (i = IndirectEntries; i >= 1; i--)
		{
			if (indirect[i].offset <= tag_table[tag_table_pos].offset)
			{
				long off = tag_table[tag_table_pos].offset - indirect[i].offset + FirstNodeOffset - 4;
				newid = openinfo(indirect[i].filename, 0);
				if (newid == NULL)
				{
					return -1;
					closeprogram();
					printf(_("Error: could not open info file part"));
					printf("\n");
					exit(1);
				}
				fclose(id);
				id = newid;
				if (off > 0)
					fseek(id, off, SEEK_SET);
				else
					fseek(id, off, SEEK_SET);
				break;
			}
		}
	}
	else
	{
		long off = tag_table[tag_table_pos].offset - 4;
		if (off > 0)
			fseek(id, off, SEEK_SET);
		else
			fseek(id, off, SEEK_SET);
	}
#undef id
	return 0;
}

/* removes trailing .gz, .bz2, etc. */
void
strip_compression_suffix(char *file)
{
	const size_t len = strlen(file);
	assert(len<1024); /* just some random limit */
	char *found = 0;

	for (unsigned j = 0; j < SuffixesNumber; j++)
	{
		if ( (found = strstr(file, suffixes[j].suffix)) != NULL )
		{
			if ( file + len == found + strlen(suffixes[j].suffix) )
			{
				*found = '\0';
				break;
			}
		}
	}
}

/* strip .info from and of string */
void
strip_info_suffix(char *file)
{
	const size_t len = strlen(file);
	assert(len<1024); /* just some random limit */

	char *found = 0;
	const char suffix[6] = ".info";

	if ( (found = strstr(file, suffix)) != NULL )
	{
		if ( file + len == found + strlen(suffix) )
		{
			*found = '\0';
		}
	}
}

