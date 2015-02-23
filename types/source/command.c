#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/command.h"

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
            for (j = i - 1; j != 0; j--)
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

void free_command(command_t *command)
{
    size_t i;
    for (i = 0; command->arguments[i]; i++)
    {
        free(command->arguments[i]);
    }
    free(command->arguments);
}
