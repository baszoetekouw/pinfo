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
#include "utils.h"
#include "tmpfiles.h"
#include <string>
using std::string;
#include <vector>
using std::vector;

vector<string> infopaths;

/******************************************************************************
 * This piece of declarations says what to do with info files stored with      *
 * different formats/compression methods, before putting them into a temporary *
 * file. I.e. you don't do anything to plain `.info' suffix; for a `.info.gz'  *
 * you dump the file through `gunzip -d -c', etc.                              *
 ******************************************************************************/

typedef struct Suffixes
{
	const char * const suffix;
	const char * const command;
}
Suffixes;

#define SuffixesNumber 4

static const Suffixes suffixes[SuffixesNumber] =
{
	{"", 		"cat"},
	{".gz",		"gzip -d -q -c"},
	{".Z",		"gzip -d -q -c"},
	{".bz2",	"bzip2 -d -c"}
};

/*****************************************************************************/

void
basename_and_dirname(const string filename, string& basename, string& dirname)
{
	/* Dirname should end with a slash, or be empty. */
	string::size_type index = filename.rfind('/');
	if (index == string::npos) {
		basename = filename;
		dirname = "";
	} else {
		basename = filename.substr(index + 1);
		dirname = filename.substr(0, index + 1);
	}
}

void
basename(const string filename, string& basename_str)
{
	string::size_type index = filename.rfind('/');
	if (index == string::npos) {
		basename_str = filename;
	} else {
		basename_str = filename.substr(index + 1);
	}
}

/* In this one, dirname *doesn't* have a trailing slash. */
void
dirname(const string filename, string& dirname_str)
{
	string::size_type index = filename.rfind('/');
	if (index == string::npos) {
		dirname_str = "";
	} else {
		dirname_str = filename.substr(0, index);
	}
}

/*
 * Strip one trailing .gz, .bz2, etc.
 * Operates in place.
 */
void
strip_compression_suffix(string& filename)
{
	for (int j = 0; j < SuffixesNumber; j++)
	{
		string::size_type suffix_len =  strlen(suffixes[j].suffix);
		if (suffix_len == 0) {
			/* Nothing is a suffix, but that gives an early false positive. */
			continue;
		}
		if (    (filename.length() >= suffix_len)
		     && (filename.compare(filename.length() - suffix_len,
		                          suffix_len, suffixes[j].suffix) == 0)
		   ) {
			/* Truncate string. */
			filename.resize(filename.length() - suffix_len);
			break;
		}
	}
}

/*
 * Strip trailing ".info" .
 * Operates in place.
 */
void
strip_info_suffix(string& filename)
{
	if (    (filename.length() >= 5)
	     && (filename.compare(filename.length() - 5, 5, ".info") == 0)
	   ) {
		/* Truncate string. */
		filename.resize(filename.length() - 5);
	}
}

void
sort_tag_table(void) {
	if (!tag_table.empty())
		std::sort(tag_table.begin(), tag_table.end(), compare_tags);
}

/*
 * Looks for name_string -- appended to buf!
 * Returns true if it finds a match, false if not.
 * Leaves the matching name in buf.
 */
bool
matchfile(string& buf, const string name_string)
{
	string basename_string;
	string dirname_string;
	basename_and_dirname(name_string, basename_string, dirname_string);

	/* remove a possible ".info" from the end of the file name
	 * we're looking for */
	strip_info_suffix(basename_string);

	/* fix the name of the dir */
	if (buf[buf.length()-1]!='/')
		buf += "/";
	buf += dirname_string;

	/* open the directory */
	DIR *dir;
	dir = opendir(buf.c_str());
	if (dir == NULL) {
		return false;
	}

	struct dirent *dp;
	/* iterate over all files in the directory */
	while ((dp = readdir(dir))) { /* Ends loop when NULL is returned */
		string test_filename = dp->d_name;

		/* strip suffixes (so "gcc.info.gz" -> "gcc") */
		strip_compression_suffix(test_filename);
		strip_info_suffix(test_filename);

		/* compare this file with the file we're looking for */
		if (test_filename  == basename_string) {
			/* Matched.  Clean up and return from function. */
			buf += "/";
			buf += test_filename;
			buf += ".info";
			closedir(dir);
			return true;
		}
	}
	closedir(dir);
	return false;
}

FILE *
dirpage_lookup(char **type, vector<string>& message,
               string wanted_name, string& first_node)
{
#define Type	(*type)
	FILE *id = 0;
	bool goodHit = false;

	id = opendirfile(0);
	if (!id)
		return 0;

	read_item(id, type, message);
	/* search for node-links in every line */
	for (int i = 0; i < message.size(); i++)	{
		/* we want: `* name:(file)node.' */
		string::size_type nameend, filestart, fileend, dot;
		if (    (message[i].length() >= 2)
		     && (message[i][0] == '*')
		     && (message[i][1] == ' ')
		     && ( (nameend = message[i].find(':')) != string::npos )
		     && (message[i].length() != nameend + 1)
		     && (message[i][nameend + 1] != ':')
		     && ( (filestart = message[i].find('(', nameend + 1)) != string::npos )
		     && ( (fileend = message[i].find(')', filestart + 1)) != string::npos )
		     && ( (dot = message[i].find('.', fileend + 1)) != string::npos )
		   ) {
			; /* Matches the pattern we want */
		} else {
			continue;
		}

		/* It looks like a match. */
		string name(message[i], 2, nameend - 2);
		string file(message[i], filestart + 1, fileend - (filestart + 1) );
		string node(message[i], fileend + 1, dot - (fileend + 1) );

		if (    (name.length() >= wanted_name.length())
		     && (strcasecmp(wanted_name.c_str(),
		                   name.substr(0, wanted_name.length()).c_str()) == 0)
	     ) {
			; /* Wanted_name begins the name, so it's a match */
		} else {
			continue;
		}

		if ( goodHit && (name.length() != wanted_name.length()) ) {
			/* skip this hit if we have already found a previous partial match,
		   * and this hit is not a perfect match */
			continue;
		}

		/* Find the name of the node link (without leading spaces) */
		if (node != "") {
			string::size_type idx = 0;
			while (isspace(node[idx]))
				idx++;
			first_node = node.substr(idx);
		}

		if (id) {
			/* Close the previously opened file */
			fclose(id);
			id = 0;
		}

		if (file.find(".info") == string::npos) {
			file += ".info";
		}

		/* See if this info file exists, and open it if it does */
		id = openinfo(file, 0);
		if (id) {
			goodHit = true;
			if ((nameend - 2) == wanted_name.length()) {
				/* the name matches perfectly to the query */
				/* stop searching for another match, and use this one */
				break;	
			}
		}
	}

	/* if we haven't found anything, clean up and exit */
	if (!goodHit)
	{
		fclose(id);
		id = 0;
	}

	/* return file we found */
	return id;
#undef Type
}

void
freeitem(char **type)
{
#define Type	(*type)
	long i;

	if (Type != 0)
	{
		xfree(Type);
		Type = 0;
	}
#undef Type
}

void
read_item(FILE * id, char **type, vector<string>& buf)
{

#define Type	(*type)

	int i;

	freeitem(type);	/* free previously allocated memory */
	buf.clear(); /* Wipe out old buffer */

	/* seek precisely on the INFO_TAG (the seeknode function may be imprecise
	 * in combination with some weird tag_tables).  */
	while (fgetc(id) != INFO_TAG);
	/* then skip the trailing `\n' */
	while (fgetc(id) != '\n');

	/* allocate and read the header line */
	Type = (char*)xmalloc(1024);
	fgets(Type, 1024, id);
	Type = (char*)xrealloc(Type, strlen(Type) + 1);

	/* now iterate over the lines until we hit a new INFO_TAG mark */
	char* tmpbuf = (char*) xmalloc(1024); /* Note, cleared like calloc */
	do {
		/* don't read after eof in info file */
		if (feof(id))
			break;

		/* Clear our buffer (needed for algorithm below) */
		memset(tmpbuf, '\0', 1024);

		/* Read a line. */
		if (fgets(tmpbuf, 1024, id) == NULL) {
			/* If there's a read error, or EOF at the start of the line,
			 * put in an empty line. */
			/* FIXME */
			strcpy(tmpbuf, "\n");
		} else {
			/* we can be sure that at least 1 char was read! */
			/* *sigh*  indices contains \0's
			 * which totally fucks up all strlen()s.
			 * so replace it by a space */
			i = 1023;
			/* find the end of the string */
			while (tmpbuf[i]=='\0' && i>=0) i--;
			/* and replace all \0's in the rest of the string by spaces */
			/* Also clean out backspaces */
			while (i>=0)
			{
				if (tmpbuf[i]=='\0' || tmpbuf[i]=='\b')
					tmpbuf[i]=' ';
				i--;
			}
		}
		string tmpstr = tmpbuf;
		buf.push_back(tmpstr);
	} while (tmpbuf[0] != INFO_TAG);	/* repeat until new node mark is found */
	xfree(tmpbuf);

	/* Note that we pushed the INFO_TAG line (or the read-zero-characters line) */
	/* -- but check the feof case, FIXME */
	/* Also, this should be dropped entirely and ismenu/isnote should be fixed */
	/* FIXME */
	/* added for simplifing two-line ismenu and isnote functs */
	if (buf.size() > 0)
	{
		buf[buf.size() - 1] = "\n";
	}

	/* Back up past that last INFO_TAG line */
	fseek(id, -2, SEEK_CUR);
#undef Type
}

void
load_indirect(vector<string> message)
{
	for (typeof(message.size()) i = 0; i < message.size(); i++) {
		/* Find the first colon, but not in position 0 */
		string::size_type n = message[i].find(':', 1);
		if (n == string::npos) {
			; /* No colon.  Invalid entry. */
		} else {
			Indirect my_entry;
			my_entry.filename = message[i].substr(0, n);
			string remainder = message[i].substr(n + 2, string::npos);
			my_entry.offset = atoi(remainder.c_str());
			indirect.push_back(my_entry);
		}
	}
}

void
load_tag_table(vector<string> message)
{
	tag_table.clear();

	if (message.size() == 0) {
		/* Fail. */
		return;
	}

	/*
	 * If the first line begins with "(indirect)", skip that line.
	 */
	bool is_indirect = false;
	if (strcasecmp("(Indirect)", message[0].substr(0, 10).c_str()) == 0)
		is_indirect = true;

	/* Run through all lines. */
	for (typeof(message.size()) i = (is_indirect ? 1 : 0);
	     i < message.size(); i++) {
		/* Skip first character and nonwhitespace after it.
		 * (Why first character? FIXME) 
		 * plus one more (space) character */
		string::size_type j;
		for (j = 1; j < message[i].size(); j++) {
			if (isspace(message[i][j])) {
				j++;
				break;
			}
		}
		if (j == message[i].size()) {
			/* No characters left for the node name. */
			continue;
		}

		/* Find INDIRECT_TAG character, but skip at least one character
		 * so the node name is nonempty */
		string::size_type ind_tag_idx = message[i].find(INDIRECT_TAG, j + 1);
		if (ind_tag_idx == string::npos) {
			continue;
		}
		TagTable my_tag;
		my_tag.nodename = message[i].substr(j, ind_tag_idx - j);
		string offset_string = message[i].substr(ind_tag_idx + 1);
		my_tag.offset = atoi(offset_string.c_str());
		tag_table.push_back(my_tag);
	}

	/* info should ALWAYS start at the 'Top' node, not at the first
	   mentioned node(vide ocaml.info) */
	for (typeof(tag_table.size()) i = 0; i < tag_table.size(); i++)
	{
		if (strcasecmp(tag_table[i].nodename.c_str(), "Top") == 0)
		{
			FirstNodeOffset = tag_table[i].offset;
			FirstNodeName = tag_table[i].nodename;
		}
	}
	sort_tag_table();
}

int
seek_indirect(FILE * id)
{
	int finito = 0;
	long seek_pos;
	int input;
	char *type = (char*)xmalloc(1024);
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
		fgets(type, 1024, id);
		if (strncasecmp("Indirect:", type, strlen("Indirect:")) == 0)
		{
			finito = 1;
		}
	}
	xfree(type);
	type = 0;
	if (!curses_open)
	{
		printf(_("Searching for indirect done"));
		printf("\n");
	}
	else
	{
		attrset(bottomline);
		mvhline(maxy - 1, 0, ' ', maxx);
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
	char *type = (char*)xmalloc(1024);
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
					if (!curses_open) {
						printf(_("Warning: could not find tag table"));
						printf("\n");
					} else
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
		while (fgetc(id) != '\n')
		{
			if (feof(id))
				break;
		}
		fgets(type, 1024, id);
		if (strncasecmp("Tag Table:", type, strlen("Tag Table:")) == 0)
		{
			finito = 1;
		}
	}
	xfree(type);
	type = 0;
	if (!curses_open)
		printf(_("Searching for tag table done\n"));
	else
	{
		attrset(bottomline);
		mvhline(maxy - 1, 0, ' ', maxx);
		mvaddstr(maxy - 1, 0, "Searching for tag table done");
		attrset(normal);
	}
	fseek(id, seek_pos, SEEK_SET);
	return 1;
}

FILE *
opendirfile(int number)
{
	FILE *id = NULL;
	string tmpfilename;
	int dir_found = 0;
	int dircount = 0;
	struct stat status;

	if (number == 0)		/* initialize tmp filename for file 1 */
	{
		if (tmpfilename1 != "")
		{
			unlink(tmpfilename1.c_str());	/* erase old tmpfile */
		}
		tmpfilename = tmpfilename1;	/* later we will refere only to tmp1 */
	}

	int *fileendentries = (int*)xmalloc(infopaths.size() * sizeof(int));
	/* go through all paths */
	for (typeof(infopaths.size()) i = 0; i < infopaths.size(); i++)	{ 
		int lang_found = 0;
		for (int k = 0; k <= 1; k++) { /* Two passes: with and without LANG */
			string bufstr;
			if (k == 0) {
				char* getenv_lang = getenv("LANG");
				/* If no LANG, skip this pass */
				if (getenv_lang == NULL)
					continue;
				bufstr = infopaths[i];
				bufstr += '/';
				bufstr += getenv_lang;
				bufstr += "/dir";
			} else { /* k == 1 */
				/* If we found one with LANG, skip this pass */
				if (lang_found)
					continue;
				bufstr = infopaths[i];
				bufstr += "/dir";
			}

			for (int j = 0; j < SuffixesNumber; j++) { /* go through all suffixes */
				string bufstr_with_suffix;
				bufstr_with_suffix = bufstr;
				bufstr_with_suffix += suffixes[j].suffix;

				id = fopen(bufstr_with_suffix.c_str(), "r");
				if (id != NULL) {
					fclose(id);
					string command_string = suffixes[j].command;
					command_string += " ";
					command_string += bufstr_with_suffix;
					command_string += ">> ";
					command_string += tmpfilename;
					system(command_string.c_str());
					lstat(tmpfilename.c_str(), &status);
					fileendentries[dircount] = status.st_size;
					dircount++;
					dir_found = 1;
					lang_found = 1;
				}
			}
		}
	}
	if (dir_found)
		id = fopen(tmpfilename.c_str(), "r");
	/*
	 * Filter the concatenated dir pages to exclude hidden parts of info
	 * entries
	 */
	if (id)
	{
		char *tmp;
		long filelen, i;
		int aswitch = 0;
		int firstswitch = 0;
		dircount = 0;

		fseek(id, 0, SEEK_END);
		filelen = ftell(id);

		tmp = (char*)xmalloc(filelen);
		fseek(id, 0, SEEK_SET);
		fread(tmp, 1, filelen, id);
		fclose(id);
		id = fopen(tmpfilename.c_str(), "w");
		for (i = 0; i < filelen; i++)
		{
			if (tmp[i] == INFO_TAG)
			{
				aswitch ^= 1;
				if (!firstswitch)
					fputc(tmp[i], id);
				firstswitch = 1;
			}
			else if ((aswitch) ||(!firstswitch))
				fputc(tmp[i], id);
			if (i + 1 == fileendentries[dircount])
			{
				if (aswitch != 0)
					aswitch = 0;
				dircount++;	/* the last dircount should fit to the end of filelen */
			}
		}
		fputc(INFO_TAG, id);
		fputc('\n', id);
		fclose(id);
		id = fopen(tmpfilename.c_str(), "r");
		xfree(tmp);

		xfree(fileendentries);
		return id;
	}
	xfree(fileendentries);
	return NULL;
}

/*
 * Note: openinfo is a function for reading info files, and putting
 * uncompressed content into a temporary filename.  For a flexibility, there
 * are two temporary files supported, i.e.  one for keeping opened info file,
 * and second for i.e. regexp search across info nodes, which are in other
 * info-subfiles.  The temporary file 1 is refrenced by number=0, and file 2 by
 * number=1 Openinfo by default first tries the path stored in
 * filenameprefix and then in the rest of userdefined paths.
 */
FILE *
openinfo(const string filename, int number)
{
	FILE *id = NULL;
	string tmpfilename;

	if (filename == "dir")
	{
		return opendirfile(number);
	}

	if (number == 0) { /* initialize tmp filename for file 1 */
		if (tmpfilename1 != "")
		{
			unlink(tmpfilename1.c_str());	/* erase old tmpfile */
		}
		tmpfilename = tmpfilename1;	/* later we will refere only to tmp1 */
	} else { /* initialize tmp filename for file 2 */
		if (tmpfilename2 != "")
		{
			unlink(tmpfilename2.c_str());	/* erase old tmpfile */
		}
		tmpfilename = tmpfilename2;	/* later we will refere only to tmp2 */
	}

	/* FIXME: signed/unsigned issues (sigh) */
	for (int i = -1; i < (int)infopaths.size(); i++) { /* go through all paths */
		string mybuf;
		if (i == -1) {
			/*
			 * no filenameprefix, we don't navigate around any specific
			 * infopage set, so simply scan all directories for a hit
			 */
			if (filenameprefix.empty())
				continue;
			else {
				mybuf = filenameprefix;
				mybuf += "/";
				string basename_string;
				basename(filename, basename_string);
				mybuf += basename_string;
			}
		} else {
			mybuf = infopaths[i];
			/* Modify mybuf in place by suffixing filename -- eeewww */
			bool result = matchfile(mybuf, filename);
			if (!result) {
				/* no match found in this directory */
				continue;
			}
		}
		for (int j = 0; j < SuffixesNumber; j++) { /* go through all suffixes */
			string buf_with_suffix = mybuf;
			buf_with_suffix += suffixes[j].suffix;
			id = fopen(buf_with_suffix.c_str(), "r");
			if (id) {
				fclose(id);
				/* Set global filenameprefix to the dirname of the found file */
				dirname(buf_with_suffix, filenameprefix);

				string command_string = suffixes[j].command;
				command_string += ' ';
				command_string += buf_with_suffix;
				command_string += "> ";
				command_string += tmpfilename;
				system(command_string.c_str());

				id = fopen(tmpfilename.c_str(), "r");
				if (id)
				{
					return id;
				}
			}
		}
		if ((i == -1) && ( !filenameprefix.empty() )) {
			/* if we have a nonzero filename prefix,
				 that is we view a set of infopages,
				 we don't want to search for a page
				 in all directories, but only in
				 the prefix directory. Therefore
				 break here. */
			break;
		}
	}
	return 0;
}

void
addrawpath(const string filename_string)
{
	/* Get the portion up to the last slash. */
	string dirstring;
	string::size_type index = filename_string.rfind('/');
	if (index != string::npos)
		dirstring = filename_string.substr(0, index + 1);
	else
		dirstring = "./"; /* If no directory part, use current directory */

	/* Add to beginning */
	infopaths.insert(infopaths.begin(), dirstring);
}

int
isininfopath(char *name)
{
	for (typeof(infopaths.size()) i = 0; i < infopaths.size(); i++)
	{
		if (infopaths[i] == name)
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
	vector<string> paths;
	char *rawlang = NULL;
	string lang;
	string langshort;
	int ret;
	size_t len;
	struct stat sbuf;

	/* first concat the paths */
	string infopath;
	char* env = getenv("INFOPATH");
	if (env != NULL)
	{
		infopath = env; 
	}
	infopath += ":"; /* FIXME: what if one of the two is blank? */
	infopath += configuredinfopath;

	/* split at ':' and put the path components into paths[] */
	string::size_type stop_idx;
	string::size_type start_idx = 0;
	do {
		stop_idx = infopath.find(':', start_idx);
		string dir;
		dir  = infopath.substr(start_idx, stop_idx - start_idx);
		/* if this actually is a non-empty string, add it to paths[] */
		if (dir.length() > 0) {
			paths.push_back(dir);
		}
		start_idx = stop_idx + 1;
	} while (stop_idx != string::npos) ;


	/* get the current $LANG, if any (to use for localized info pages) */
	rawlang = getenv("LANG");
	if (rawlang) {
		lang = rawlang;
		/* fix the lang string */
		/* cut off the charset */
		string::size_type idx = lang.find('.');
		if (idx != string::npos) {
			lang.resize(idx);
		}
		/* if lang is sublocalized (nl_BE or so), also use short version */
		idx = lang.find('_');
		if (idx != string::npos) {
			langshort = lang;
			langshort.resize(idx);
		}
	}

	/* if we have a LANG defined, add paths with this lang to the paths[] */
	if (lang != "") {
		typeof(paths.size()) old_size = paths.size();
		if (langshort != "") {
			paths.resize(old_size * 3);
		} else {
			paths.resize(old_size * 2);
		}
		for (typeof(paths.size()) i=0; i<old_size; i++) {
			string tmp;
			tmp = paths[i];
			tmp += '/';
			tmp += lang;
			/* add the lang specific dir at the beginning */
			paths[old_size+i] = paths[i];
			paths[i] = tmp;
			
			if (langshort != "") {
				string tmp;
				tmp = paths[i];
				tmp += '/';
				tmp += langshort;
				paths[2*old_size+i] = paths[old_size+i];
				paths[old_size+i] = tmp;
			}
		}
	}

#ifdef ___DEBUG___
	/* for debugging */
	for (typeof(paths.size()) i=0; i<paths.size(); i++)
		fprintf(stderr,"--> %s\n", paths[i].c_str());
#endif

	/* ok, now we have all the (possibly) revelevant paths in paths[] */
	/* now loop over them, see if they are valid and if they are duplicates*/
	vector<ino_t> inodes;
	int numpaths = 0;
	len = 0;
	for (typeof(paths.size()) i=0; i<paths.size(); i++) {
		inodes.push_back(0);
		/* stat() the dir */
		ret = stat( paths[i].c_str(), &sbuf);
		/* and see if it could be opened */
		if (ret < 0)
		{
#ifdef ___DEBUG___
			fprintf(stderr, "error while opening `%s': %s\n",
					paths[i].c_str(), strerror(errno) );
#endif
			paths[i] = "";
			inodes[i] = 0;
		}
		else
		{
			inodes[i] = sbuf.st_ino;
		}

		/* now check if this path is a duplicate */
		for (typeof(paths.size()) j=0; j<i; j++)
		{
			if (inodes[j]==inodes[i])
				paths[i] = "";
		}

		/* calculate the total number of vali paths and the size of teh strings */
		if (paths[i]!="")
		{
			numpaths++;
			len += paths[i].length() + 1;
		}
	}


	infopaths.clear();
	for (typeof(paths.size()) i=0; i<paths.size(); i++)
	{
		if (paths[i]!="")
		{
			infopaths.push_back(paths[i]);
		}
	}

#ifdef ___DEBUG___
	/* for debugging */
	fprintf(stderr, "%i valid info paths found:\n", infopaths.size());
	for (typeof(paths.size()) i=0; i<infopaths.size(); i++)
		if (infopaths[i] != "") fprintf(stderr,"--> %s\n", infopaths[i].c_str());
#endif
}

void
create_indirect_tag_table()
{
	FILE *id = 0;
	int initial;
	for (typeof(indirect.size()) i = 0; i < indirect.size(); i++)
	{
		id = openinfo(indirect[i].filename, 1);
		initial = tag_table.size(); /* Before create_tag_table operates */
		if (id)
		{
			create_tag_table(id);
			FirstNodeOffset = tag_table[0].offset;
			FirstNodeName = tag_table[0].nodename;
		}
		fclose(id);
		for (typeof(tag_table.size()) j = initial; j < tag_table.size(); j++)
		{
			tag_table[j].offset +=(indirect[i].offset - FirstNodeOffset);
		}
	}
	FirstNodeOffset = tag_table[0].offset;
	FirstNodeName = tag_table[0].nodename;
	sort_tag_table();
}

void
create_tag_table(FILE * id)
{
	char *buf = (char*)xmalloc(1024);
	long oldpos;
	fseek(id, 0, SEEK_SET);
	while (!feof(id))
	{
		if (fgetc(id) == INFO_TAG)	/* We've found a node entry! */
		{
			while (fgetc(id) != '\n');	/* skip '\n' */
			oldpos = ftell(id);	/* remember this file position! */
			/*
			 * it is a an eof-fake-node (in some info files it happens, that
			 * the eof'ish end of node is additionaly signalised by an INFO_TAG
			 * We give to such node an unlike to meet nodename.
			 */
			if (fgets(buf, 1024, id) == NULL) {
				TagTable my_tag;
				my_tag.nodename = "12#!@#4";
				my_tag.offset = 0;
				tag_table.push_back(my_tag);
			} else {
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
								TagTable my_tag;
								buf[j] = 0;
								buflen = j;
								my_tag.nodename = buf + i + 2;
								my_tag.offset = oldpos - 2;
								tag_table.push_back(my_tag);
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
	if (indirect.empty()) /* originally (!indirect) -- check this NCN FIXME */
	{
		FirstNodeOffset = tag_table[0].offset;
		FirstNodeName = tag_table[0].nodename;
		sort_tag_table();
	}
}

void
seeknode(int tag_table_pos, FILE ** Id)
{
	int i;
#define id	(*Id)
	/*
	 * Indirect nodes are seeked using a formula:
	 * file-offset = tagtable_offset - indirect_offset +
	 *             + tagtable[1]_offset
	 */
	if (!(indirect.empty()))	/* Originally if (indirect) -- NCN CHECK FIXME */
	{
		for (i = indirect.size() - 1; i >= 0; i--)
		{
			if (indirect[i].offset <= tag_table[tag_table_pos].offset)
			{
				long off = tag_table[tag_table_pos].offset - indirect[i].offset + FirstNodeOffset - 4;
				fclose(id);
				id = openinfo(indirect[i].filename, 0);
				if (id == NULL)
				{
					closeprogram();
					printf(_("Error: could not open info file\n"));
					exit(1);
				}
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
}

