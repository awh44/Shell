#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SUCCESS      0
#define READ_ERROR   1
#define LINE_EMPTY   2
#define MEMORY_ERROR 4
#define FORK_ERROR   8
#define EXEC_ERROR   16

#define HISTORY_LENGTH 10

#define MIN(a, b) (a) < (b) ? (a) : (b)

typedef int status_code;
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
} history_t;

unsigned short eval_print(char *line, size_t size, history_t *history);
status_code parse_line(char *line, size_t chars_read, char ***arguments, size_t *argc, unsigned short *background);
void trim(char *line, size_t length);
char **split(char *s, char delim, size_t *number);
status_code execute(char **arguments, unsigned short background);
void add_to_history(history_t *history, char **arguments, size_t argc, unsigned short background);
void clear_history_entry(history_t *history, size_t index);
void clear_history(history_t *history);
void print_history(history_t *history);
void handle_error(status_code error_code);

int main(int argc, char *argv[])
{
	int cont = 1;
	char *line = NULL;
	size_t size;
	history_t history = {0};

	while (cont)
	{
		fprintf(stdout, "osh> ");
		fflush(stdout);
		ssize_t chars_read = getline(&line, &size, stdin);
		if (chars_read < 0)
		{
			handle_error(READ_ERROR);
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
	else if (strcmp(line, "history\n") == 0)
	{
		print_history(history);
		return 1;
	}
	else if (strcmp(line, "!!\n") == 0 && history->num_commands > 0)
	{
		command_t command = history->commands[(history->num_commands - 1) % HISTORY_LENGTH];
		execute(command.arguments, command.background);
		add_to_history(history, command.arguments, command.argc, command.background);
		return 1;
	}

	char **arguments = NULL;
	size_t argc = 0;
	unsigned short background;
	status_code error = parse_line(line, chars_read, &arguments, &argc, &background);
	if (error != SUCCESS)
	{
		handle_error(error);
		return 1;
	}

	error = execute(arguments, background);
	if (error == EXEC_ERROR)
	{
		//child process could not execute execvp, so free its memory and exit
		handle_error(error);
		free(arguments);
		free(line);
		clear_history(history);
		exit(1);
	}
	else if (error != SUCCESS)
	{
		handle_error(error);
	}

	add_to_history(history, arguments, argc, background);

	//arguments must either be NULL or must not have been freed by one of the subsequently
	//called functions, so safe to just free it here.	
	free(arguments);
	return 1;
}

status_code parse_line(char *line, size_t chars_read, char*** arguments, size_t *argc, unsigned short *background)
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
	*arguments = split(line, ' ', argc);
	if (*arguments == NULL)
	{
		return MEMORY_ERROR;
	}
	
	//determine if the process should be run in the background
	*background = 0;
	if (strcmp((*arguments)[*argc - 1], "&") == 0)
	{
		*background = 1;
		(*arguments)[*argc - 1] = NULL;
	}
	else
	{
		(*argc)++;
		char **tmp = realloc(*arguments, *argc * sizeof *tmp);
		if (tmp == NULL)
		{
			return MEMORY_ERROR;
		}
		
		*arguments = tmp;
		(*arguments)[*argc - 1] = NULL;
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

char **split(char *s, char delim, size_t *number)
{
    char **ret_val = NULL;
    *number = 0;
    char *start_pos = s;
    size_t i;
    for (i = 0; s[i]; i++)
    {
        if (s[i] == delim || s[i + 1] == '\0')
        {
            (*number)++;
			char **tmp = realloc(ret_val, *number * sizeof *ret_val);
			if (tmp == NULL)
			{
				free(ret_val);
				return NULL;
			}

			ret_val = tmp;
            if (s[i] == delim)
			{
				s[i] = '\0';
			}
            ret_val[*number - 1] = start_pos;

            start_pos = s + i + 1;
        }
    }
    
	return ret_val;
}

status_code execute(char **arguments, unsigned short background)
{
	pid_t pid = fork();
	if (pid < 0)
	{
		return FORK_ERROR;
	}
	else if (pid == 0)
	{
		//have child execute the desired program
		if (execvp(arguments[0], arguments) < 0)
		{
			return EXEC_ERROR;
		}
	}
	else if (!background)
	{
		int status;
		waitpid(pid, &status, NULL);
	}

	return SUCCESS;
}

void add_to_history(history_t *history, char **arguments, size_t argc, unsigned short background)
{
	size_t index = history->num_commands % HISTORY_LENGTH;
	if (history->num_commands >= HISTORY_LENGTH)
	{
		clear_history_entry(history, index);
	}
	history->num_commands++;

	history->commands[index].number = history->num_commands;

	char **new_array = malloc(argc * sizeof *new_array);
	size_t i;
	for (i = 0; i < argc - 1; i++)
	{
		new_array[i] = strdup(arguments[i]);
	}
	new_array[i] = NULL;

	history->commands[index].arguments = new_array;
	history->commands[index].background = background;
	history->commands[index].argc = argc;
}

void clear_history_entry(history_t *history, size_t index)
{
	size_t i;
	for (i = 0; history->commands[index].arguments[i]; i++)
	{
		free(history->commands[index].arguments[i]);
	}
	free(history->commands[index].arguments);
}

void clear_history(history_t *history)
{
	size_t min = MIN(history->num_commands, HISTORY_LENGTH);
	size_t i;
	for (i = 0; i < min; i++)
	{
		clear_history_entry(history, i);
	}
}

void print_history(history_t *history)
{
	size_t num_to_do = history->num_commands < HISTORY_LENGTH ? history->num_commands : HISTORY_LENGTH;
	ssize_t start_index = history->num_commands % HISTORY_LENGTH;
	size_t i;
	for (i = 1; i <= num_to_do; i++)
	{
		ssize_t current_index = (start_index - (ssize_t) i) % HISTORY_LENGTH;
		current_index += current_index < 0 ? HISTORY_LENGTH : 0;
		fprintf(stdout, "%zu", history->commands[current_index].number);
		size_t j;
		for (j = 0; history->commands[current_index].arguments[j]; j++)
		{
			fprintf(stdout, " %s", history->commands[current_index].arguments[j]);
		}

		if (history->commands[current_index].background)
		{
			fprintf(stdout, " &");
		}
		fprintf(stdout, "\n");
	}		
}


void handle_error(status_code error_code)
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
		default:
			fprintf(stderr, "Error: Unknown error.");
	}
	fprintf(stderr, "\n");
}
