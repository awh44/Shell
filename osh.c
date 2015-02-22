#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SUCCESS          0
#define READ_ERROR       1
#define LINE_EMPTY       3
#define MEMORY_ERROR     4
#define FORK_ERROR       5
#define EXEC_ERROR       6
#define NO_COMMANDS      7
#define NO_EXIST_ERROR   8
#define NUMBER_ERROR     9
#define ARGS_ERROR    10
#define CD_ERROR        11

#define HISTORY_LENGTH 10
#define ALIAS_BUCKETS  128

#define ASCII_0 48
#define ASCII_9 57

#define MIN(a, b) (a) < (b) ? (a) : (b)

typedef int status_t;

typedef struct
{
	size_t number;
	char **arguments;
	size_t argc;
	unsigned short background;
} command_t;

typedef struct
{
	command_t commands[HISTORY_LENGTH];
	size_t num_commands;
	size_t length;
} history_t;

typedef struct
{
	char *alias;
	command_t *command;
} alias_t;

typedef struct
{
	alias_t aliases[ALIAS_BUCKETS];
	size_t buckets;
} alias_table_t;

typedef struct
{
	char **path;
	history_t *history;
	alias_table_t *aliases;
} environment_t;

static alias_table_t tmp_table;


unsigned short eval_print(char *line, size_t size, history_t *history);

//PARSING FUNCTIONS
status_t parse_line(char *line, size_t chars_read, command_t *command);
void trim(char *line, size_t length);
char **split(char *s, char delim, unsigned short retain_quotes, size_t *number);
//----------------

//EXECUTION FUNCTIONS
status_t execute_builtin(history_t *history, command_t *command, unsigned short *is_builtin);
status_t execute(history_t *history, command_t *command);
//------------------

//COMMAND FUNCTIONS
status_t copy_command(command_t *destination, command_t *source);
void print_command(command_t *command);
void free_command(command_t *command);
//-----------------

//HISTORY and COMMAND FUNCTIONS
void add_to_history(history_t *history, command_t *command);
void clear_history(history_t *history);
void print_history(history_t *history);
//-----------------------------

//ALIAS FUNCTIONS
status_t add_alias(alias_table_t *table, char *name, char *command);
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
	int cont = 1;
	char *line = NULL;
	size_t size;
	history_t history = {0};
	history.length = HISTORY_LENGTH;

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
			cont = eval_print(line, chars_read, &history);
		}
	}

	free(line);
	clear_history(&history);
	return 0;
}

unsigned short eval_print(char *line, size_t chars_read, history_t *history)
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

	unsigned short is_builtin;
	error = execute_builtin(history, &command, &is_builtin);
	if (!is_builtin)
	{
		error = execute(history, &command);
	}

	if (error == EXEC_ERROR)
	{
		//child process could not execute execvp, so free its memory and exit
		error_message(error);
		free(command.arguments);
		free(line);
		clear_history(history);
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

	size_t i;
	for (i = 0; i < command->argc; i++)
	{
		printf("%s\n", command->arguments[i]);
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

status_t execute_builtin(history_t *history, command_t *command, unsigned short *is_builtin)
{
	if (strcmp(command->arguments[0], "history") == 0)
	{
		print_history(history);
		*is_builtin = 1;
		return SUCCESS;
	}

	if (command->arguments[0][0] == '!')
	{
		*is_builtin = 1;
		if (command->arguments[0][1] == '!')
		{
			if (history->num_commands >= 1)
			{
				command_t *old_command = &history->commands[(history->num_commands - 1) % history->length];
				print_command(old_command);
				return execute(history, old_command);
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
			printf("number = %zu, index = %zu\n", number, index);
			command_t *old_command = &history->commands[index];
			print_command(old_command);
			return execute(history, old_command);
		}
	}

	if (strcmp(command->arguments[0], "cd") == 0)
	{
		*is_builtin = 1;
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

	if (strcmp(command->arguments[0], "alias") == 0)
	{
		*is_builtin = 1;
		
		//one for "alias", one for the new name, one for the command, one for the NULL pointer
		if (command->argc < 4)
		{
			return ARGS_ERROR;
		}

		status_t error = add_alias(&tmp_table, command->arguments[1], command->arguments[2]);
		return error;
	}

	*is_builtin = 0;
	return SUCCESS;
}

status_t execute(history_t *history, command_t *command)
{
	pid_t pid = fork();
	if (pid < 0)
	{
		add_to_history(history, command);
		return FORK_ERROR;
	}
	
	if (pid == 0)
	{
		//have child execute the desired program
		if (execvp(command->arguments[0], command->arguments) < 0)
		{
			return EXEC_ERROR;
		}
	}
	
	//parent case - child process will either be replaced or will return (in the case of an error),
	//so will never reach here
	add_to_history(history, command);
	//if it's now a background command, then don't wait for it
	if (!command->background)
	{
		int status;
		waitpid(pid, &status, 0);
	}

	return SUCCESS;
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
		}
	}
	new_array[i] = NULL;
	destination->arguments = new_array;
	destination->argc = source->argc;
	destination->background = source->background;
}

void add_to_history(history_t *history, command_t *command)
{
	size_t index = history->num_commands % history->length;
	history->num_commands++;

	//save the old command's values, in case it needs to be freed later
	command_t old_command = history->commands[index];

	copy_command(&history->commands[index], command);
	history->commands[index].number = history->num_commands;

	if (history->num_commands > history->length)
	{
		free_command(&old_command);
	}
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
		print_command(&history->commands[current_index]);
	}		
}

void print_command(command_t *command)
{
	fprintf(stdout, "%zu", command->number);
	size_t j;
	for (j = 0; command->arguments[j]; j++)
	{
		fprintf(stdout, " %s", command->arguments[j]);
	}

	if (command->background)
	{
		fprintf(stdout, " &");
	}

	fprintf(stdout, "\n");
}

status_t add_alias(alias_table_t *table, char *name, char *command)
{
	//remove the quotes from the command
	command++;
	size_t chars_read = strlen(command);
	command_t new_command;
	status_t error = parse_line(command, chars_read, &new_command);
	if (error != SUCCESS)
	{
		return error;
	}

	return SUCCESS;
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
