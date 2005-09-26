/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
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

#include <string>
using std::string;

#include <iostream> // for ostringstream and cerr
using std::cerr;
#include <sstream> // for ostringstream
using std::ostringstream;


/* For mkdir, umask */
#include <sys/stat.h>

/* For getpid, mkdir, umask */
#include <sys/types.h>

/* For getpid */
#include <unistd.h>

/* For errno */
#include <errno.h>

/* For strerror */
#include <string.h>

string tmpdirname;
string tmpfilename1;
string tmpfilename2;
string apropos_tmpfilename;

void mktmpdir () {
	ostringstream tmpdirname_stream;
	tmpdirname_stream << "/tmp/pinfo";
	pid_t my_pid = getpid();
	tmpdirname_stream << my_pid;
	tmpdirname = tmpdirname_stream.str();
	if (tmpdirname == "/tmp/pinfo") {
		cerr << "Unexpected error: Couldn't get temporary directory name.\n";
		exit(1);
	}

	int result;
	result = mkdir(tmpdirname.c_str(), 0700);
	if (result != 0) {
		int errval = errno;
		cerr << "Error: couldn't make temporary directory " << tmpdirname
		     << " (" << strerror(errval) << ")\n";
		exit(1);
	}

	tmpfilename1 = tmpdirname;
	tmpfilename1 += "/current-file";
	tmpfilename2 = tmpdirname;
	tmpfilename2 += "/cross-file";

	/* FIXME: This belongs elsewhere. */
	/* Most of our temp files are created by calls to system().
	 * We need to set the umask so that it will be inherited.
	 */
	umask(077);
}

void rmtmpfiles () {
	int result;

	result = unlink(tmpfilename1.c_str());
	/* It might not exist; this is sloppy.  FIXME later. */

	result = unlink(tmpfilename2.c_str());
	/* It might not exist; this is sloppy.  FIXME later. */

	if (apropos_tmpfilename != "") {
		result = unlink(apropos_tmpfilename.c_str());
		/* It might not exist; this is sloppy.  FIXME later. */
	}

	result = rmdir(tmpdirname.c_str());
	if (result != 0) {
		int errval = errno;
		cerr << "Error: couldn't remove temporary directory " << tmpdirname
		     << " (" << strerror(errval) << ")\n";
	}
	tmpdirname = "";
}
