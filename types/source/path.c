#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/path.h"

unsigned short dir_accessible(char *dir);

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

void print_path(path_t *path)
{
    size_t i;
    for (i = 0; i < path->num_dirs; i++)
    {
        fprintf(stdout, "%s\n", string_c_str(path->dirs + i));
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
