# Shell
An implemenation of a basic Unix shell for an operating systems course. The following functionality
has been implemented and can be found in the given files.

#Directory Structure
First, just a quick note about directory structure. There are several types that were defined for
this project, and their type/'class' definitions can be found in src/types/include, and their
implementations can be found in src/types/source. Also note that analagous directories,
src/misc/include and src/misc/source, exist for miscellaneous parsing functions. Below, the word
definition is used to interchangeably to refer to these different files, but note that the actual
definition is in the include/ while the implementation is in the source/.

#Run Commands
Including foreground and background commands. This functionality is essentially found in src/osh.c
and src/misc/source/parse.c. The latter file does all of the command line parsing, using the
command\_t type found at src/types/source/command.c, and osh.c actually performs the execution of
external programs, using the execute\+external function, which eventually calls execv. Background
and foreground commands are supported.

#History
The history\_t type is defined in src/types/source/history.c, but it is manipulated by src/osh.c.
(Whenever an external program is executed by execute\_external, the command is added to the
history.) The history\_command function in src/osh.c handles parsing of the user input for !! and
!integer style commands, but all other functionality is implemented in src/types/source/history.c.

#Aliasing
Adding and listing aliases is almost completely handled by src/types/source/alias.c, with a little
help from src/osh.c in execute\_builtin and subsequently alias\_command. Executing aliased commands
is done by alias\_execute\_command in osh.c. Note that aliasing does not work with pipes right now.
It is, however, possible to add more arguments to an alias. For example, imagine the alias dir "ls
-a". It is possible to write dir -l, to add more flags to the end of the command.

#Scripting
This is handled entirely by src/osh.c. The environment\_t type (defined in
src/types/source/environment.c) maintains information about which, if any, file is open, but
script\_command and endscript\_command in osh.c handle all the needed functionality here. Note that
scripting might play a little funny with background commands, because there is no synchronization
done on who gets to write to the file at any given time.

#Path
A path type is defined in src/types/source/path.c, and after command line parsing
(src/misc/source/parse.c and src/osh.c, including set\_command and set\_path\_command), handles
setting the path. Search of the path is done in src/osh.c, in the execute\_child function, as called
by execute\_external.

#Initialization File
This is completely handled by the initialize\_shell function in src/osh.c.

#Verbosity
This is handled completely by sr/osh.c After command line parsing, execute\_builtin, set\_command,
and set\_verbose\_command handle setting the correct values in the environment\_t.

#Pipes
The parsing of the command line required for pipes is handled by the various functions in
src/misc/source/parse.c. These functions in turn manipulate the command type, as found in
src/types/source/command.c, setting up a linked list of piped commands. Finally, in child\_execute
in src/osh.c, the commands are actually set up to pipe into one another.

#Change Directory
This is handled by the cd\_command function in src/osh.c

Please refer to the function names and the JavaDoc-style function and struct comments to further
refine where to look, given these bases.
