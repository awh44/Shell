#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS      0
#define READ_ERROR   1
#define LINE_EMPTY   2
#define MEMORY_ERROR 4
#define FORK_ERROR   8
#define EXEC_ERROR   16

typedef int status_code;

void read_evaluate_print(FILE *stream);
status_code parse_line(char *line, size_t chars_read, char ***arguments, int *background);
void trim(char *line, size_t length);
char **split(char *s, char delim, size_t *number);
status_code execute(char **arguments, int background); 
void handle_error(status_code error_code);

int main(int argc, char *argv[])
{
	int cont = 1;
	char *line = NULL;
	size_t size;

	while (cont)
	{
		fprintf(stdout, "osh> ");
		fflush(stdout);

		ssize_t chars_read = getline(&line, &size, stdin);
		if (chars_read < 0)
		{
			handle_error(READ_ERROR);
		}
		else if (strcmp(line, "exit\n") != 0)
		{
			char **arguments = NULL;
			int background;
			status_code error = parse_line(line, chars_read, &arguments, &background);
			if (error != SUCCESS)
			{
				handle_error(error);
			}
			else
			{
				error = execute(arguments, background);
				if (error == EXEC_ERROR)
				{
					//child process could not execute execvp, so free its memory and exit
					handle_error(error);
					free(arguments);
					free(line);
					exit(1);
				}
				else if (error != SUCCESS)
				{
					handle_error(error);
				}
			}

			//arguments must either be NULL or must not have been freed by one of the subsequently
			//called functions, so safe to just free it here.
			free(arguments);
		}
		else
		{
			cont = 0;
		}
	}

	free(line);
	return 0;
}

status_code parse_line(char *line, size_t chars_read, char*** arguments, int *background)
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
	size_t argc;
	*arguments = split(line, ' ', &argc);
	if (*arguments == NULL)
	{
		return MEMORY_ERROR;
	}
	
	//determine if the process should be run in the background
	*background = 0;
	if (strcmp((*arguments)[argc - 1], "&") == 0)
	{
		*background = 1;
		argc--;
		(*arguments)[argc] = NULL;
	}
	else
	{
		char **tmp = realloc(*arguments, (argc + 1) * sizeof *tmp);
		if (tmp == NULL)
		{
			return MEMORY_ERROR;
		}
		
		*arguments = tmp;
		(*arguments)[argc] = NULL;
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

status_code execute(char **arguments, int background)
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
