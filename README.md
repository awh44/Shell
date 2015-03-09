# Shell
An implemenation of a basic Unix shell for an operating systems course. The following functionality
has been implemented and can be found in the given files. To compile the shell, type "make osh".
To run the shell, type "make" or "make run". To quit the shell, type "exit" at the prompt.

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
and foreground commands are supported. Searches the path in order to find the entered command. If
the entered command contains slashes, the shell first tries that command alone, assuming it to be a
full path.

#History
The history\_t type is defined in src/types/source/history.c, but it is manipulated by src/osh.c.
(Whenever an external program is executed by execute\_external, the command is added to the
history.) The history\_command function in src/osh.c handles parsing of the user input for !! and
!integer style commands, but all other functionality is implemented in src/types/source/history.c.
Note that bulitin commands are NOT added to the history, by concious choice. Setting an alias,
setting a path, setting verbosity, or starting or ending a script did not seem like things that
would need to be executed again, so they are not tracked in the history. The one other thing to note
here is that when an aliased command is executed, the *actual* command, and not the alias, is placed
in the history. This allows the user to see what they are actually executing, and it also simplifies
the execution of that command again.

#Aliasing
Adding and listing aliases is almost completely handled by src/types/source/alias.c, with a little
help from src/osh.c in execute\_builtin and subsequently alias\_command. Executing aliased commands
is done by alias\_execute\_command in osh.c. Note that aliasing does not work with pipes right now.
It is, however, possible to add more arguments to an alias. For example, imagine the alias dir "ls
-a". It is possible to write dir -l, to add more flags to the end of the command. As with the
example, aliases must use quotes around the desired command to be aliased.

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
and set\_verbose\_command handle setting the correct values in the environment\_t. Currently,
verbosity prints three things. One, it echos the command back to the user after the input has been
parsed; two, it outputs all of the directories in the path after a set path; and three, it outputs
the directories being checked as it searches the path for a command.

#Pipes
The parsing of the command line required for pipes is handled by the various functions in
src/misc/source/parse.c. These functions in turn manipulate the command type, as found in
src/types/source/command.c, setting up a linked list of piped commands. Finally, in child\_execute
in src/osh.c, the commands are actually set up to pipe into one another. Note that there is
something strange with the way the parent process waits for pipe commands to finish. That is, the
initial shell process waits for its child to finish, but it does not wait for any further children
to finish. As noted in the alias section, pipes do not currently work with aliases.

#Change Directory
This is handled by the cd\_command function in src/osh.c

Please refer to the function names and the JavaDoc-style function and struct comments to further
refine where to look, given these bases.
