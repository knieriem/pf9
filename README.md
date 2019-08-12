_Pf9_ (package framework 9) is a port of some libraries and programs
from [_Plan9 from User Space_](https://9fans.github.io/plan9port/) to
Win32 using the MinGW-w64 environment. A lot of files are used without
modification, some need only slight adaptions. The actual porting has
been done in lib9/mingw and libthread/mingw.c, with help of ideas from
inferno and 9pm.

See `LICENSE` file for which licenses apply to the source files.

For a list of ported programs see DIRS and TARG definitions
in src/cmd/mkfile.

For a list of ported libraries see src/lib*. Status of some
libraries:

-	libthread: works mostly, daemonizing works not

-	libdraw:	works basically, tested with tcolors, acme
		and sam. Events have not been ported, so only
		programs using the thread-based mouse
		interface will run.


Status of the support of various concepts:

*	Pipes - Implemented using Named Pipes.

*	Unix Sockets  
	Implemented using Named Pipes. You can use the same
	addresses as in plan9port, e.g. unix!/tmp/acme.

*	Internet Sockets  
	Implemented using Winsock.

*	Unicode  
	All programs get UTF-8 encoded streams on stdin, UTF-8
	encoded environment and command line arguments.

*	Console support  
	Reading from the console input and writing to the console
	screen buffer is supported transparently. A `cat > file` from
	the Windows console will produce UTF-8 characters, regardless
	of which codepage is installed.
	
	You can also use the history feature of the console window without
	extra support in programs.

Because the mkfile system's structure has been derived from Plan9, you
can compile the libs and sources for Linux and MinGW in a similar way:
On Linux, to compile for the MinGW, run

	cd src
	objtype=mingw
	mk


Copy identical files from Plan9port
---------------------------

A lot of files are used without changes from a specific revision
of plan9port.  They are not distributed along with pf9. This copy of
plan9port can be setup using

	git clone https://github.com/9fans/plan9port
	cd plan9port
	git checkout 1889a257
	PLAN9=`pwd`
	export PLAN9

If you have, in parallel, a current version of plan9port installed,
which has already been added to PATH, these steps should be
sufficient. Otherwise you would have to run `./INSTALL` in the checked
out copy of plan9port, so that some commands like `sed`, or `date`
get built, that are needed for the pf9 build process.

You can run

	mk pop		- to populate the source directory with identical
					source files from p9p

	mk rm		... remove identical source files; this undos the
				effect of `mk pop'

	mk tkdiff		... to get a list of tkdiff commands you can click at in rc to
				explore the differences between p9p and pf9

	mk eqdiff		... like `mk tkdiff`, but this time a list showing on which
				files an ed script has been run.

Some files will be modified slightly after copying from plan9port. This
is done using one ed script per file.


How to build for MinGW-w64 (Win32)
----------------------------------

The MinGW-w64 compiler and development files for
Win32 must be available, which can, for example, be
installed on Debian using the [`gcc-mingw-w64-i686`
package](https://packages.debian.org/buster/gcc-mingw-w64-i686).

1.	Set `PF9` to the main directory of this distribution. Set cputype
	and objtype:
		cputype=lin386
		objtype=mingw

	You might just run `sh misc/enterpf9` alternatively, which should
	start an rc shell with these values already set (it won't affect the
	environment of the calling shell).
	
2.	Set PATH to include the necessary bin directories $cputype/bin and
	$cpusys/bin, i.e. lin386/bin and linux/bin. They should come
	before the PLAN9/bin directory, to avoid conflicts. If you have
	run `misc/enterpf9` in step 1, this has already been done.

3.	You should have run `mk pop` once to copy identical source files from
	your plan9port distribution, and to apply changes, where neccessary.

4.	Build libraries and programs:

		cd src
		mk
		mk install
	
	On success you will the programs in mingw/bin.

To get a distribution for windows, e.g. the compiled binaries
and additional files like troff fonts from plan9port, look into
`dist/mingw/mkfile`.
This needs some further explanation.



Run it
-----

FIXME
	
This should be enough to get it work. Now you might generate the
lookman index from a shell prompt, which is a good test of some
of the functionality:

	$ rc
	% cd $PF9/sys/man
	% ./mkindex


Debugging
--------

Define `FDTDEBUG=2` to get debug output from the file descriptor layer.
