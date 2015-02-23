#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_t.h"
#define HISTORY_LENGTH 10
#define ALIAS_BUCKETS  128

#define ASCII_0 48
#define ASCII_9 57

#define INITIALIZE_FILE "/.cs543rc"
/*
 * Define the error constants. These are kind of muddled, because they are a mixture of user (e.g.,
 * the user input a floating point where an integer was expected) and system (e.g., could not fork a
 * new process), but they work fine.
 */
#define SUCCESS          0
#define READ_ERROR       1
#define LINE_EMPTY       3
#define MEMORY_ERROR     4
#define FORK_ERROR       5
#define EXEC_ERROR       6
#define NO_COMMANDS      7
#define NO_EXIST_ERROR   8
#define NUMBER_ERROR     9
#define ARGS_ERROR      10
#define CD_ERROR        11
#define EXISTS_ERROR    12
#define FORMAT_ERROR    13
#define OPEN_ERROR      14
#define ALREADY_OPEN    15
#define DUP2_ERROR      16
#define NOT_OPEN        17
#define INVALID_VAR     18
#define DIR_ERROR       19

#define MIN(a, b) (a) < (b) ? (a) : (b)

/**
  * An error type. Returned from functions to indicate what type of error occurred; generally one of
  * the types #define'd above
  */
typedef uint64_t status_t;

/**
  * A struct holding information about a command, including its command number, the arguments, the
  * number of arguments, and whether it is to execute in the background.
  */
typedef struct
{
	size_t number;
	char **arguments;
	size_t argc;
	unsigned short background;
} command_t;

/**
  * A struct to hold an array of the path directories, along with the number of direcotires in the
  * path
  */
typedef struct
{
	string_t *dirs;
	size_t num_dirs;
} path_t;

/**
  * A struct holding information about the command history, including an array of commands going a
  * set length back in history, the total number of commands executed, and the length of the
  * commands array
  */
typedef struct
{
	command_t commands[HISTORY_LENGTH];
	size_t num_commands;
	size_t length;
} history_t;

/**
  * Holds information about an alias, including the name of the alias and the command associated
  * with that name. Also includes a next pointer for use in a linked-list/hash table
  */
typedef struct alias_t
{
	char *alias;
	command_t *command;
	struct alias_t *next;
} alias_t;

/**
  * A hash table for the aliases.
  */
typedef struct
{
	alias_t *alias_entries[ALIAS_BUCKETS];
} alias_table_t;

/**
  * Holds all of the information about the user's current environment, including their path
  * variable, their history, their aliases, and any open script file, with plenty room for any more
  * to come
  */
typedef struct
{
	path_t *path;
	history_t *history;
	alias_table_t *aliases;
	int script_file;
	unsigned short verbose;
} environment_t;


void initialize_shell(environment_t *environment);
/**
  * Given a line of a particular size, evaluates/executes the resulting command in the given
  * environment, returning whether the program should continue or not after this function as a
  * boolean value
  * @param line        the line to be evaluated
  * @param size        the size of the line
  * @param environment the current environment in which to evaluate
  * @return whether the program should continue executing
  */
unsigned short eval_print(char *line, size_t size, environment_t *environment);

//PARSING FUNCTIONS
/**
  * Parses the given line made up of chars_read characters, setting the values of command in doing
  * so. Note that this function does NOT allocate new memory for each element of the command's
  * arguments array. That is, each element of arguments points at a part of the pre-existing line.
  * the arguments array is also NULL terminated, and argc counts the NULL pointer as an argument.
  * @param line       the line to be parsed
  * @param chars_read the number of characters read from the command line (i.e., the length of line)
  * @param command    out param; arguments is NULL terminated and elements set to diff. pos. of line
  * @return a status code indicating whether an error occurred during execution of the function
 */
status_t parse_line(char *line, size_t chars_read, command_t *command);

/**
  * Trims, in place, the leading and trailing white space of the line with length length
  * @param line   the line to be trimmed
  * @param length the length of the line
  */
void trim(char *line, size_t length);

/**
  * Splits the given line on the character delim, not splitting on quotes if retain_quotes is true,
  * and returning an array of pointers INTO s. In other words, the function does not allocate new
  * strings for the elements of the returned array, and s is destroyed upon a call to this function.
  * @param s             the string to split; this string is not constant over this function
  * @param delim         the character on which to split
  * @param retain_quotes if true, if a delim is found within quotes, the func will not split on it
  * @param number        out param; indicates hwo many elements s split into
  * @return returns an array of elements split on delim; each element is NOT newly allocated but is
  * simply a pointer into s
  */
char **split(char *s, char delim, unsigned short retain_quotes, size_t *number);
//----------------

//EXECUTION FUNCTIONS
/**
  * If command is a builtin command, this function will execute it, setting is_builtin appropriately
  * @param environment the current environment in which to execute the command
  * @param command     the command to be executed
  * @param is_builtin  out param; is set to true if the command is actually a builtin, false if not
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t execute_builtin(environment_t *environment, command_t *command, unsigned short *is_builtin);
/**
  * Execute sthe command indicated by command, in the current environment
  * @param environment the current environment in which to execute the command
  * @param command     the command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t execute_external(environment_t *environment, command_t *command);
//------------------

//BUILTIN COMMAND HANDLERS
/**
  * Handles a history command (one executed by !! or !integer), executing if possible and returning
  * an error otherwise
  * @param environment the current environment in which to execute the command
  * @param command     the history command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t history_command(environment_t *environment, command_t *command);

/**
  * Handles a cd command, returning an error code if an error occurs
  * @param command     the cd command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t cd_command(command_t *command);

/**
  * Handles an alias command (i.e., "alias [name] "command"), executing if possible and returning an
  * error otherwise
  * @param environment the current environment in which to execute the alias command 
  * @param command     the alias command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t alias_command(environment_t *environment, command_t *command);

/**
  * Handles an alias execute command (i.e., the execution of a previously defined alias), executing
  * of possible and returning an error otherwise
  * @param environment the current environment in which to execute the aliased command
  * @param command     the original command as given by the user
  * @param alias       the aliased command
  */
status_t alias_execute_command(environment_t *environment, command_t *original, command_t *alias);

/**
  * Handles a script command (i.e., "script [scriptname]"), starting the script if possible and
  * returning an error otherwise. Sets the appropriate variables in environment as needed.
  * @param environment the current environment in which to begin the script
  * @param command     the script command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t script_command(environment_t *environment, command_t *command);

/**
  * Handles an endscript command, ending the script and closing the file if possible, returning an
  * error otherwise. Sets the appropriate variables in envrionment and closes files as needed.
  * @param environment the current environment in which to end the script
  * @param command     the endscript command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t endscript_command(environment_t *environment, command_t *command);

/**
  * Handles a "set", setting the correct variable in the environment if possible, returning an error
  * otherwise.
  * @param environment the current environment to set the variable into
  * @param command     the set command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t set_command(environment_t *environment, command_t *command);

/**
  * Handles a "set path", setting the correct variable in the environment if possible, returning an error
  * otherwise.
  * @param environment the current environment to set the path variable into
  * @param command     the set path command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t set_path_command(environment_t *environment, command_t *command);

/**
  * Handles a "set verbose", setting the correct variable in the environment if possible, returning an error
  * otherwise.
  * @param environment the current environment to set the verbose variable into
  * @param command     the set verbose command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t set_verbose_command(environment_t *environment, command_t *command);
//-----------------------

//COMMAND FUNCTIONS
status_t copy_command(command_t *destination, command_t *source);
void print_command(command_t *command);
void free_command(command_t *command);
//-----------------

//ENVIRONMENT FUNCTIONS
void clear_environment(environment_t *environment);
//---------------------

//PATH FUNCTIONS
status_t set_path(path_t *path, command_t *command);
status_t resize_initialize_path(path_t *path, size_t num_dirs);
unsigned short dir_accessible(char *dir);
void clear_path(path_t *path);
void print_path(path_t *path);
//--------------

//HISTORY FUNCTIONS
status_t add_to_history(history_t *history, command_t *command);
void clear_history(history_t *history);
void print_history(history_t *history);
//-----------------------------

//ALIAS FUNCTIONS
status_t add_alias(alias_table_t *table, char *name, char *command, unsigned short overwrite);
void print_aliases(alias_table_t *table);
alias_t *find_alias(alias_table_t *table, char *alias);
void remove_alias(alias_table_t *table, char *entry);
void clear_aliases(alias_table_t *table);
size_t alias_hash(char *name);
//--------------

//ERROR FUNCTION
void error_message(status_t error_code);
//-------------

//HELPER FUNCTIONS
status_t convert(char *s, size_t *value);
size_t hash(char *str);
//----------------

int main(int argc, char *argv[])
{
	path_t path = {0};
	history_t history = {0};
	history.length = HISTORY_LENGTH;
	alias_table_t aliases = {0};
	environment_t environment = { &path, &history, &aliases, -1, 0 };
	initialize_shell(&environment);

	int cont = 1;
	char *line = NULL;
	size_t size = 0;

	while (cont)
	{
		fprintf(stdout, "osh> ");
		fflush(stdout);
		ssize_t chars_read = getline(&line, &size, stdin);
		if (chars_read < 0)
		{
			error_message(READ_ERROR);
		}
		else
		{
			cont = eval_print(line, chars_read, &environment);
		}
	}

	free(line);
	clear_environment(&environment);
	return 0;
}

void initialize_shell(environment_t *environment)
{
	string_t file_dir;
	string_initialize(&file_dir);
	string_assign_from_char_array(&file_dir, getenv("HOME"));
	string_concatenate_char_array(&file_dir, INITIALIZE_FILE);
	FILE *file = fopen(string_c_str(&file_dir), "r");
	string_uninitialize(&file_dir);
	if (file == NULL)
	{
		error_message(OPEN_ERROR);
		return;
	}

	char *line = NULL;
	size_t size = 0;
	ssize_t chars_read = getline(&line, &size, file);
	while (chars_read > 0)
	{
		eval_print(line, chars_read, environment);
		chars_read = getline(&line, &size, file);
	}

	free(line);
	fclose(file);
}

unsigned short eval_print(char *line, size_t chars_read, environment_t *environment)
{
	if (strcmp(line, "exit\n") == 0)
	{
		return 0;
	} 

	command_t command = {0};
	status_t error = parse_line(line, chars_read, &command);
	if (error != SUCCESS)
	{
		error_message(error);
		return 1;
	}

	if (environment->verbose)
	{
		print_command(&command);
		fprintf(stdout, "\n");
	}

	unsigned short is_builtin;
	error = execute_builtin(environment, &command, &is_builtin);
	if (!is_builtin)
	{
		error = execute_external(environment, &command);
	}

	if (error == EXEC_ERROR || error == DUP2_ERROR)
	{
		//child process could not execute execv, so free its memory and exit
		error_message(error);
		free(command.arguments);
		free(line);
		clear_environment(environment);
		//if the process made it to EXEC, it must have already closed the script_file, if it was
		//open, so it doesn't have to be closed here
		exit(1);
	}
	else if (error != SUCCESS)
	{
		error_message(error);
	}


	//arguments must either be NULL or must not have been freed by one of the subsequently
	//called functions, so safe to just free it here.	
	free(command.arguments);
	return 1;
}

status_t parse_line(char *line, size_t chars_read, command_t *command)
{
	//handle case of empty line
	if (chars_read <= 1)
	{
		return LINE_EMPTY;
	}

	//get rid of the newline by shortening the string by a character
	chars_read--;
	line[chars_read] = '\0';

	//get rid of leading and trailing whitespace
	trim(line, chars_read);

	//split the line into the arguments array
	command->arguments = split(line, ' ', 1, &command->argc);
	if (command->arguments == NULL)
	{
		return MEMORY_ERROR;
	}

	//determine if the process should be run in the background
	command->background = 0;
	if (strcmp(command->arguments[command->argc - 1], "&") == 0)
	{
		command->background = 1;
		command->arguments[command->argc - 1] = NULL;
	}
	else
	{
		command->argc++;
		char **tmp = realloc(command->arguments, command->argc * sizeof *tmp);
		if (tmp == NULL)
		{
			return MEMORY_ERROR;
		}
		
		command->arguments = tmp;
		command->arguments[command->argc - 1] = NULL;
	}

	return SUCCESS;
}

void trim(char *line, size_t length)
{
	size_t spaces = 0;
	while (line[spaces] == ' ')
	{
		spaces++;
	}

	size_t end = length - 1;
	if (spaces != 0)
	{
		memmove(line, line + spaces, length - spaces);
		end = length - spaces - 1;
		line[end + 1] = '\0';
	}

	while (line[end] == ' ')
	{
		end--;
	}

	line[end + 1] = '\0';
}

char **split(char *s, char delim, unsigned short retain_quotes, size_t *number)
{
	*number = 0;
    char **ret_val = NULL;
	unsigned short in_quotes = 0;
    char *start_pos = s;
    size_t i;
    for (i = 0; s[i]; i++)
    {
		if (s[i] == '"')
		{
			in_quotes = !in_quotes;
		}

		if ((s[i] == delim && !in_quotes) || (s[i + 1] == '\0'))
		{
			//reallocate space for another element in the array
			(*number)++;
			char **tmp = realloc(ret_val, *number * sizeof *ret_val);
			//if space can't be allocated, free the memory allocated so far and return NULL
			if (tmp == NULL)
			{
				free(ret_val);
				return NULL;
			}
			//otherwise, update the array's value
			ret_val = tmp;

			//next, indicate that the string starts at the given position. If it's the case where a
			//delimiter has been found, insert a null terminator to split the string up
			if (s[i] == delim)
			{
				s[i] = '\0';
			}
			ret_val[*number - 1] = start_pos;

			//update the starting position to be 1 after the delimiter	
			start_pos = s + i + 1;
        }
    }
    
	return ret_val;
}

status_t execute_builtin(environment_t *environment, command_t *command, unsigned short *is_builtin)
{
	//assume command is a builtin
	*is_builtin = 1;
	if (strcmp(command->arguments[0], "history") == 0)
	{
		print_history(environment->history);
		return SUCCESS;
	}

	if (command->arguments[0][0] == '!')
	{
		return history_command(environment, command);
	}

	if (strcmp(command->arguments[0], "cd") == 0)
	{
		return cd_command(command);
	}

	if (strcmp(command->arguments[0], "alias") == 0)
	{
		return alias_command(environment, command);
	}

	alias_t *alias;
	//if the command is an alias, then execute it now
	if ((alias =  find_alias(environment->aliases, command->arguments[0])) != NULL)
	{
		return alias_execute_command(environment, command, alias->command);
	}

	if (strcmp(command->arguments[0], "script") == 0)
	{
		return script_command(environment, command);
	}

	if (strcmp(command->arguments[0], "endscript") == 0)
	{
		return endscript_command(environment, command);
	}

	if (strcmp(command->arguments[0], "set") == 0)
	{
		return set_command(environment, command);
	}

	//if made it to here, command is not a builtin
	*is_builtin = 0;
	return SUCCESS;
}

status_t history_command(environment_t *environment, command_t *command)
{
	history_t *history = environment->history;
	if (command->arguments[0][1] == '!')
	{
		if (history->num_commands >= 1)
		{
			command_t *old_command = &history->commands[(history->num_commands - 1) % history->length];
			print_command(old_command);
			fprintf(stdout, "\n");
			return execute_external(environment, old_command);
		}
		else
		{
			return NO_COMMANDS;
		}
	}
	else
	{
		size_t number;
		status_t error = convert(command->arguments[0] + 1, &number);
		if (error != SUCCESS)
		{
			return error;
		}
	
		ssize_t min_number = (ssize_t) history->num_commands - (ssize_t) history->length + 1;
		if ((number > history->num_commands) || (ssize_t) number < min_number || number == 0)
		{
			return NO_EXIST_ERROR;
		}

		size_t index = (number - 1) % history->length;
		command_t *old_command = &history->commands[index];
		print_command(old_command);
		fprintf(stdout, "\n");
		return execute_external(environment, old_command);
	}
}

status_t cd_command(command_t *command)
{
	//one for "cd", one for the directory, one for the NULL pointer
	if (command->argc < 3)
	{
		return ARGS_ERROR;
	}

	if (chdir(command->arguments[1]) < 0)
	{
		return CD_ERROR;
	}

	return SUCCESS;
}

status_t alias_command(environment_t *environment, command_t *command)
{
	//one for "alias", one for NULL pointer
	if (command->argc == 2)
	{
		//print all the aliases
		print_aliases(environment->aliases);
		return SUCCESS;
	}
	
	//one for "alias", one for the new name, one for the command, one for the NULL pointer
	if (command->argc < 4)
	{
		return ARGS_ERROR;
	}

	//don't allow aliases that contain slahses - these will muddle with the search of the path
	//variable
	if (strchr(command->arguments[1], '/') != NULL)
	{
		return FORMAT_ERROR;
	}

	return add_alias(environment->aliases, command->arguments[1], command->arguments[2], command->argc > 4);
}

status_t alias_execute_command(environment_t *environment, command_t *original, command_t *alias)
{
	//one for alias command and one for NULL pointer
	if (original->argc == 2 && !original->background)
	{
		return execute_external(environment, alias);
	}

	command_t expanded;
	expanded = *alias;
	size_t new_argc = original->argc - 2 + alias->argc;
	expanded.arguments = malloc(new_argc * sizeof *expanded.arguments);
	memcpy(expanded.arguments, alias->arguments, alias->argc * sizeof *alias->arguments);
	size_t new_index, old_index = 1;
	for (new_index = alias->argc - 1; new_index < new_argc; new_index++)
	{
		expanded.arguments[new_index] = original->arguments[old_index];
		old_index++;
	}
	
	if (original->background)
	{
		expanded.background = 1;
	}
	
	status_t error = execute_external(environment, &expanded);
	free(expanded.arguments);
	
	return error;
}


status_t script_command(environment_t *environment, command_t *command)
{
	if (environment->script_file >= 0)
	{
		return ALREADY_OPEN;
	}

	//one for "script", one for filename, one for NULL
	if (command->argc < 3)
	{
		return ARGS_ERROR;
	}

	int fd = open(command->arguments[1], O_CREAT | O_WRONLY, 0600);
	if (fd < 0)
	{
		return OPEN_ERROR;
	}

	environment->script_file = fd;
	return SUCCESS;
}

status_t endscript_command(environment_t *environment, command_t *command)
{
	if (environment->script_file < 0)
	{
		return NOT_OPEN;
	}

	close(environment->script_file);
	environment->script_file = -1;
	return SUCCESS;
}

status_t set_command(environment_t *environment, command_t *command)
{
	//one for "set", one for type, one for NULL pointer
	if (command->argc < 3)
	{
		return ARGS_ERROR;
	}

	if (strcmp(command->arguments[1], "path") == 0)
	{
		status_t error = set_path_command(environment, command);
		if (error == SUCCESS && environment->verbose)
		{
			print_path(environment->path);
		}

		return error;
	}

	if (strcmp(command->arguments[1], "verbose") == 0)
	{
		return set_verbose_command(environment, command);
	}

	return INVALID_VAR;
}

status_t set_path_command(environment_t *environment, command_t *command)
{
	//one for "set", one for "path", one for "=", one+ for path value, one for NULL pointer
	if (command->argc < 5)
	{
		return ARGS_ERROR;
	}

	//next argument after "path" must be an equals sign
	if (strcmp(command->arguments[2], "=") != 0)
	{
		return FORMAT_ERROR;
	}

	//next argument must begin with a left parenthesis and must be at least two characters long
	//additionally, cannot have command of type set path = () (i.e., no empty path)
	size_t first_length = strlen(command->arguments[3]);
	if (command->arguments[3][0] != '(' || command->arguments[3][1] == ')' || first_length < 2)
	{
		return FORMAT_ERROR;
	}

	//last argument must end with a parenthesis and must also be at least two characters long
	size_t last_length = strlen(command->arguments[command->argc - 2]);
	if (command->arguments[command->argc - 2][last_length - 1] != ')' || last_length < 2)
	{
		return FORMAT_ERROR;
	}

	return set_path(environment->path, command);
}

status_t set_verbose_command(environment_t *environment, command_t *command)
{
	//one for "set", one for "verbose", one for "on/off", one for NULL pointer
	if (command->argc < 4)
	{
		return ARGS_ERROR;
	}

	if (strcmp(command->arguments[2], "on") == 0)
	{
		environment->verbose = 1;
		return SUCCESS;
	}

	if (strcmp(command->arguments[2], "off") == 0)
	{
		environment->verbose = 0;
		return SUCCESS;
	}

	return FORMAT_ERROR;
}

status_t execute_external(environment_t *environment, command_t *command)
{
	pid_t pid = fork();
	if (pid < 0)
	{
		add_to_history(environment->history, command);
		return FORK_ERROR;
	}
	
	if (pid == 0)
	{
		if (environment->script_file >= 0)
		{
			int result = dup2(environment->script_file, 1);
			if (result < 0)
			{
				return DUP2_ERROR;
			}
			
			result = dup2(environment->script_file, 2);
			if (result < 0)
			{
				return DUP2_ERROR;
			}

			fprintf(stdout, "\n");
			print_command(command);
			fprintf(stdout, "\n");
			close(environment->script_file);
			environment->script_file = -1;
		}
		//have child execute the desired program
		//try the command alone by itself first, so that full paths can work
		if (environment->verbose)
		{
			fprintf(stdout, "Trying to execute at path %s...\n", command->arguments[0]);
		}
		execv(command->arguments[0], command->arguments);

		//if that does not work, then try appending the command to all of the directories in the
		//path, in order
		size_t i;
		for (i = 0; i < environment->path->num_dirs; i++)
		{
			string_concatenate_char_array(environment->path->dirs + i, command->arguments[0]);
			char *c_str = string_c_str(environment->path->dirs + i);
			if (environment->verbose)
			{
				fprintf(stdout, "Trying to execute at path %s...\n", c_str);
			}
			execv(c_str, command->arguments);
		}

		return EXEC_ERROR;
	}
	
	//parent case - child process will either be replaced or will return (in the case of an error),
	//so will never reach here
	status_t error = add_to_history(environment->history, command);
	//if it's now a background command, then don't wait for it
	if (!command->background)
	{
		int status;
		waitpid(pid, &status, 0);
	}

	return error;
}

status_t copy_command(command_t *destination, command_t *source)
{
	destination->number = source->number;
	char **new_array = malloc(source->argc * sizeof *destination->arguments);
	if (new_array == NULL)
	{
		return MEMORY_ERROR;
	}

	size_t i;
	for (i = 0; i < source->argc - 1; i++)
	{
		new_array[i] = strdup(source->arguments[i]);
		if (new_array[i] == NULL)
		{
			size_t j;
			for (j = i - 1; j >= 0; j--)
			{
				free(new_array[j]);
			}
			free(new_array);
			return MEMORY_ERROR;
		}
	}
	new_array[i] = NULL;

	destination->arguments = new_array;
	destination->argc = source->argc;
	destination->background = source->background;

	return SUCCESS;
}

status_t add_to_history(history_t *history, command_t *command)
{
	size_t index = history->num_commands % history->length;
	history->num_commands++;

	//save the old command's values, in case it needs to be freed later
	command_t old_command = history->commands[index];

	status_t error = copy_command(&history->commands[index], command);
	if (error != SUCCESS)
	{
		return error;
	}
	history->commands[index].number = history->num_commands;

	if (history->num_commands > history->length)
	{
		free_command(&old_command);
	}

	return SUCCESS;
}

void free_command(command_t *command)
{
	size_t i;
	for (i = 0; command->arguments[i]; i++)
	{
		free(command->arguments[i]);
	}
	free(command->arguments);
}

void clear_environment(environment_t *environment)
{
	clear_path(environment->path);
	clear_history(environment->history);
	clear_aliases(environment->aliases);
	if (environment->script_file >= 0)
	{
		close(environment->script_file);
	}
	environment->script_file = -1;
}

status_t set_path(path_t *path, command_t *command)
{
	//one for "set", one for "path", one for "=", one for NULL pointer
	status_t error = resize_initialize_path(path, command->argc - 4);
	if (error != SUCCESS)
	{
		return error;
	}

	size_t last_length = strlen(command->arguments[command->argc - 2]);
	size_t current_index = 0;
	size_t i;
	for (i = 0; i < path->num_dirs; i++)
	{
		//adjust the argument to deal with the parentheses; do the num_dirs - 1 case first to handle
		//the case of only one directory in the path correctly (otherwise, the ')' wouldn't be
		//eliminated correctly)
		char *argument = command->arguments[i + 3];
		if (i == path->num_dirs - 1)
		{
			argument[last_length - 1] = '\0';
		}

		if (i == 0)
		{
			argument++;
		}
			
		if (dir_accessible(argument))
		{
			string_assign_from_char_array(path->dirs + current_index, argument);
			//add a backslash no matter what, because any number of slashes is equivalent to one
			string_concatenate_char_array(path->dirs + current_index, "/");
			current_index++;
		}
		else
		{
			fprintf(stderr, "Error: Could not add %s to the path.\n", argument);
		}

		if (i == path->num_dirs - 1)
		{
			argument[last_length - 1] = ')';
		}

	}

	//uninitialize any strings that will now be unused (because directories could not be used in the
	//path)
	for (i = current_index; i < path->num_dirs; i++)
	{
		string_uninitialize(path->dirs + i);
	}

	path->num_dirs = current_index;
	path->dirs = realloc(path->dirs, path->num_dirs * sizeof *path->dirs);
	
	return SUCCESS;
}

status_t resize_initialize_path(path_t *path, size_t num_dirs)
{
	if (num_dirs > path->num_dirs)
	{	
		path->dirs = realloc(path->dirs, num_dirs * sizeof *path->dirs);
		size_t i;
		for (i = path->num_dirs; i < num_dirs; i++)
		{
			string_initialize(path->dirs + i);
		}
	}
	else if (num_dirs < path->num_dirs)
	{
		size_t i;
		for (i = num_dirs; i < path->num_dirs; i++)
		{
			string_uninitialize(path->dirs + i);
		}
		path->dirs = realloc(path->dirs, num_dirs * sizeof *path->dirs);
	}

	path->num_dirs = num_dirs;
	return SUCCESS;
}

unsigned short dir_accessible(char *dir)
{
	DIR *test_dir;
	if ((test_dir = opendir(dir)) == NULL)
	{
		return 0;
	}
	else
	{
		closedir(test_dir);
		return 1;
	}
}

void clear_path(path_t *path)
{
	size_t i;
	for (i = 0; i < path->num_dirs; i++)
	{
		string_uninitialize(path->dirs + i);
	}
	free(path->dirs);
}

void print_path(path_t *path)
{
	size_t i;
	for (i = 0; i < path->num_dirs; i++)
	{
		fprintf(stdout, "%s\n", string_c_str(path->dirs + i));
	}
}

void clear_history(history_t *history)
{
	size_t min = MIN(history->num_commands, history->length);
	size_t i;
	for (i = 0; i < min; i++)
	{
		free_command(&history->commands[i]);
	}
}

void print_history(history_t *history)
{
	size_t num_to_do = MIN(history->num_commands, history->length);
	ssize_t start_index = history->num_commands % history->length;
	size_t i;
	for (i = 1; i <= num_to_do; i++)
	{
		ssize_t current_index = (start_index - (ssize_t) i) % (ssize_t) history->length;
		current_index += current_index < 0 ? history->length : 0;
		fprintf(stdout, "%zu ", history->commands[current_index].number);
		print_command(&history->commands[current_index]);
		fprintf(stdout, "\n");
	}		
}

void print_command(command_t *command)
{
	if (command->argc < 1)
	{
		return;
	}

	fprintf(stdout, "%s", command->arguments[0]);
	size_t j;
	for (j = 1; command->arguments[j]; j++)
	{
		fprintf(stdout, " %s", command->arguments[j]);
	}

	if (command->background)
	{
		fprintf(stdout, " &");
	}
}

void print_aliases(alias_table_t *table)
{
	size_t i;
	for (i = 0; i < ALIAS_BUCKETS; i++)
	{
		alias_t *entry = table->alias_entries[i];
		while (entry != NULL)
		{
			fprintf(stdout, "%s: ", entry->alias);
			print_command(entry->command);
			fprintf(stdout, "\n");
			entry = entry->next;
		}
	}
}

status_t add_alias(alias_table_t *table, char *name, char *command, unsigned short overwrite)
{
	alias_t *entry = find_alias(table, name);
	if (entry != NULL)
	{
		if (!overwrite)
		{
			return EXISTS_ERROR;
		}

		remove_alias(table, name);
	}

	//remove the quote from the command
	if (command[0] != '"')
	{
		return FORMAT_ERROR;
	}
	command++;
	size_t chars_read = strlen(command);

	if (command[chars_read - 1] != '"')
	{
		return FORMAT_ERROR;
	}

	command_t new_command;
	status_t error = parse_line(command, chars_read, &new_command);
	if (error != SUCCESS)
	{
		return error;
	}

	command_t *command_copy = malloc(sizeof *command_copy);
	if (command_copy == NULL)
	{
		free(new_command.arguments);
		return MEMORY_ERROR;
	}

	error = copy_command(command_copy, &new_command);
	free(new_command.arguments);
	if (error != SUCCESS)
	{
		free(command_copy);
		return error;
	}

	alias_t *new_alias = malloc(sizeof *new_alias);
	if (new_alias == NULL)
	{
		free(command_copy);
		return MEMORY_ERROR;
	}
	new_alias->command = command_copy;
	new_alias->alias = strdup(name);
	if (new_alias->alias == NULL)
	{
		free(command_copy);
		free(new_alias);
		return MEMORY_ERROR;
	}

	size_t hash_val = alias_hash(name);
	new_alias->next = table->alias_entries[hash_val];
	table->alias_entries[hash_val] = new_alias;

	return SUCCESS;
}

alias_t *find_alias(alias_table_t *table, char *alias)
{
	size_t hash_val = alias_hash(alias);
	alias_t *entry = table->alias_entries[hash_val];
	while (entry != NULL)
	{
		if (strcmp(entry->alias, alias) == 0)
		{
			return entry;
		}

		entry = entry->next;
	}

	return NULL;
}

void remove_alias(alias_table_t *table, char *name)
{
	size_t hash_val = alias_hash(name);
	alias_t *entry = table->alias_entries[hash_val];

	if (entry == NULL)
	{
		return;
	}

	//handle case where alias is head of the linked list
	if (strcmp(entry->alias, name) == 0)
	{
		alias_t *to_be_freed = entry;
		table->alias_entries[hash_val] = to_be_freed->next;
		free_command(to_be_freed->command);
		free(to_be_freed->command);
		free(to_be_freed->alias);
		free(to_be_freed);
		return;
	}

	//handle case where alias is deeper in list
	while (entry != NULL)
	{
		if (entry->next && strcmp(entry->next->alias, name) == 0)
		{
			alias_t *to_be_freed = entry->next;
			entry->next = to_be_freed->next;
			free_command(to_be_freed->command);
			free(to_be_freed->command);
			free(to_be_freed->alias);
			free(to_be_freed);
			return;
		}
		entry = entry->next;
	}	
}

void clear_aliases(alias_table_t *table)
{
	size_t i;
	for (i = 0; i < ALIAS_BUCKETS; i++)
	{
		alias_t *entry = table->alias_entries[i];
		while (entry != NULL)
		{
			alias_t *tmp = entry->next;
			free_command(entry->command);
			free(entry->command);
			free(entry->alias);
			free(entry);
			entry = tmp;
		}
	}
}

size_t alias_hash(char *name)
{
	return hash(name) % ALIAS_BUCKETS;
}

void error_message(status_t error_code)
{
	switch (error_code)
	{
		case READ_ERROR:
			fprintf(stderr, "\nError: Could not read.");
			break;
		case LINE_EMPTY:
			break;
		case MEMORY_ERROR:
			fprintf(stderr, "Error: Could not allocate memory.");
			break;
		case FORK_ERROR:
			fprintf(stderr, "Error: Could not fork.");
			break;
		case EXEC_ERROR:
			fprintf(stderr, "Error: Could not execute the program.");
			break;
		case NO_COMMANDS:
			fprintf(stderr, "Error: No commands in history.");
			break;
		case NO_EXIST_ERROR:
			fprintf(stderr, "Error: That command does not exist in the history.");
			break;
		case NUMBER_ERROR:
			fprintf(stderr, "Error: Number formatted incorrectly.");
			break;
		case ARGS_ERROR:
			fprintf(stderr, "Error: Not enough arguments included.");
			break;
		case CD_ERROR:
			fprintf(stderr, "Error: Could not change to that directory.");
			break;
		case EXISTS_ERROR:
			fprintf(stderr, "Error: That alias already exists.");
			break;
		case FORMAT_ERROR:
			fprintf(stderr, "Error: Incorrect format.");
			break;
		case OPEN_ERROR:
			fprintf(stderr, "Error: Could not open file.");
			break;
		case ALREADY_OPEN:
			fprintf(stderr, "Error: Script file already open.");
			break;
		case DUP2_ERROR:
			fprintf(stderr, "Error: Could not map file to stdout or stderr.");
			break;
		case NOT_OPEN:
			fprintf(stderr, "Error: No script file currently open.");
			break;
		case INVALID_VAR:
			fprintf(stderr, "Error: That value cannot currently be set.");
			break;
		case DIR_ERROR:
			fprintf(stderr, "Error: Could not add directory to path: ");
			break;
		default:
			fprintf(stderr, "Error: Unknown error.");
	}
	fprintf(stderr, "\n");
}

status_t convert(char *s, size_t *value)
{
	*value = 0;
	size_t length = strlen(s);
	size_t power10;
	size_t i;
	for (i = length - 1, power10 = 1; i < SIZE_MAX; i--, power10 *= 10)
	{
		if (s[i] < ASCII_0 || s[i] > ASCII_9)
		{
			return NUMBER_ERROR;
		}
		*value += (s[i] - ASCII_0) * power10;
	}

	return SUCCESS;
}

/*
	djb2 algorithm - see http://www.cse.yorku.ca/~oz/hash.html
*/
size_t hash(char *str)
{
	size_t hash_val = 5381;
	char c;

	while (c = *str++)
	{
		hash_val = ((hash_val << 5) + hash_val) + c;
	}

	return hash_val;
}
