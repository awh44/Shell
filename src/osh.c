#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "misc/include/parse.h"
#include "types/include/alias.h"
#include "types/include/command.h"
#include "types/include/environment.h"
#include "types/include/history.h"
#include "types/include/path.h"
#include "types/include/status.h"
#include "types/include/string_t.h"

#define ASCII_0 48
#define ASCII_9 57

#define INITIALIZE_FILE "/.cs543rc"

/**
  * Initializes the shell, executing any commands in the user's .cs543rc file and placing any
  * resulting changes into the given environment
  * @param environment out param (essentially); all initializiation executed in this environment
  */
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

/**
  * If command is a builtin command, this function will execute it, setting is_builtin appropriately
  * @param environment the current environment in which to execute the command
  * @param command     the command to be executed
  * @param is_builtin  out param; is set to true if the command is actually a builtin, false if not
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t execute_builtin(environment_t *environment, command_t *command, unsigned short *is_builtin);

/**
  * Executes the command indicated by command, in the current environment
  * @param environment the current environment in which to execute the command
  * @param command     the command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t execute_external(environment_t *environment, command_t *command);

/**
  * Perform the actual execution by the child process of the command
  * @param environment the current environment in which to execute the command
  * @param command     the command to be executed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t child_execute(environment_t *environment, command_t *command);

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
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t endscript_command(environment_t *environment);

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

/**
  * Converts a string pointed to by s to a size_t, setting *value on success and returning an error
  * otherwise
  * @param s     the string to be converted
  * @param value out param; where the converted value will be placed
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t convert(char *s, size_t *value);

int main(void)
{
	//initialize the envrionment on the stack, in main
	path_t path = {0};
	history_t history = {0};
	history.length = HISTORY_LENGTH;
	alias_table_t aliases = {0};
	environment_t environment = { &path, &history, &aliases, -1, 0 };
	
	//open the user's initialization function to further set up the shell
	initialize_shell(&environment);

	int cont = 1;
	char *line = NULL;
	size_t size = 0;

	//enter REPL loop
	while (cont)
	{
		fprintf(stdout, "osh> ");
		fflush(stdout);
		ssize_t chars_read = getline(&line, &size, stdin);
		if (chars_read < 0)
		{
			cont = 0;
			fprintf(stdout, "\n");
			//error_message(READ_ERROR);
		}
		else
		{
			cont = eval_print(line, chars_read, &environment);
		}
	}

	//cleanup
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
	//enter "REPL" loop
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
	//user wants to exit, return that indication to caller
	if (strcmp(line, "exit\n") == 0)
	{
		return 0;
	} 

	//parse the input and place the result in the given command
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

	//if command is a builtin, let execute_builtin handle it, otherwise execute the external command
	unsigned short is_builtin;
	error = execute_builtin(environment, &command, &is_builtin);
	if (!is_builtin)
	{
		error = execute_external(environment, &command);
	}

	if (error == EXEC_ERROR || error == DUP_ERROR || error == DUP2_ERROR || error == PIPE_ERROR || error == CHILD_FORK_ERR)
	{
		//child process could not execute execv, so free its memory and exit
		error_message(error);
		free(command.arguments);
		free(line);
		clear_environment(environment);
		exit(1);
	}
	else if (error != SUCCESS)
	{
		//some other error occurred (in the parent)
		error_message(error);
		free_linked_list(command.pipe);
		free(command.arguments);
		return 1;
	}


	//Only made it here if no errors occurred, so safe to free command.arguments
	free_linked_list(command.pipe);
	free(command.arguments);
	return 1;
}

status_t execute_builtin(environment_t *environment, command_t *command, unsigned short *is_builtin)
{
	//assume command is a builtin; if function makes it to end, then reset it
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
		return endscript_command(environment);
	}

	if (strcmp(command->arguments[0], "set") == 0)
	{
		return set_command(environment, command);
	}

	//if made it to here, command is not a builtin
	*is_builtin = 0;
	return SUCCESS;
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
		return child_execute(environment, command);
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

status_t child_execute(environment_t *environment, command_t *command)
{
	FILE *verbose_out = stdout;
	if (command->pipe != NULL)
	{
		int pipefd[2];
		if (pipe(pipefd) < 0)
		{
			return PIPE_ERROR;
		}

		pid_t child_fork_pid = fork();
		if (child_fork_pid < 0)
		{
			close(pipefd[0]);
			close(pipefd[1]);
			return CHILD_FORK_ERR;
		}

		if (child_fork_pid == 0)
		{
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[1]);
			return child_execute(environment, command->pipe);
		}

		dup2(pipefd[1], STDOUT_FILENO);
	}
	else if (environment->script_file >= 0)
	{
		//don't write verbose output to the script file - write to actual stdout still
		if ((verbose_out = fdopen(dup(STDOUT_FILENO), "w")) == NULL)
		{
			return DUP_ERROR;
		}

		//copy stdout to the file
		int result = dup2(environment->script_file, STDOUT_FILENO);
		if (result < 0)
		{
			fclose(verbose_out);
			return DUP2_ERROR;
		}
		
		//and copy stderr too
		result = dup2(environment->script_file, STDERR_FILENO);
		if (result < 0)
		{
			fclose(verbose_out);
			return DUP2_ERROR;
		}

		//print the command to the script file
		fprintf(stdout, "\n");
		print_command(command);
		fprintf(stdout, "\n");

		//close the script file and set the file descriptor to -1, indicating that it has been
		//closed
		close(environment->script_file);
		environment->script_file = -1;
	}

	//have child execute the desired program
	//follow execvp rules - if the command contains a slash, try that full path by itself first
	if (strchr(command->arguments[0], '/') != NULL)
	{
		if (environment->verbose)
		{
			fprintf(verbose_out, "Trying to execute at path %s\n", command->arguments[0]);
		}

		execv(command->arguments[0], command->arguments);
	}

	//if that does not work, then try appending the command to all of the directories in the
	//path, in order, and then try to execute
	size_t i;
	for (i = 0; i < environment->path->num_dirs; i++)
	{
		string_concatenate_char_array(environment->path->dirs + i, command->arguments[0]);
		char *c_str = string_c_str(environment->path->dirs + i);
		if (environment->verbose)
		{
			fprintf(verbose_out, "Trying to execute at path %s\n", c_str);
		}
		execv(c_str, command->arguments);
	}

	if (verbose_out != stdout)
	{
		fclose(verbose_out);
	}

	return EXEC_ERROR;
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
	expanded.argc = original->argc - 2 + alias->argc;
	expanded.arguments = malloc(expanded.argc * sizeof *expanded.arguments);
	memcpy(expanded.arguments, alias->arguments, alias->argc * sizeof *alias->arguments);
	size_t new_index, old_index = 1;
	for (new_index = alias->argc - 1; new_index < expanded.argc; new_index++)
	{
		expanded.arguments[new_index] = original->arguments[old_index];
		old_index++;
	}

	if (original->background)
	{
		expanded.background = 1;
	}

	status_t error;/* = setup_pipes(&expanded);
	if (error != SUCCESS)
	{
		return error;
	}*/

	error = execute_external(environment, &expanded);
	free(expanded.arguments);
	//free_linked_list(expanded.pipe);
	
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

status_t endscript_command(environment_t *environment)
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
