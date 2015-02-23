#ifndef __PATH__H__
#define __PATH__H__

#include "string_t.h"
#include "status.h"
#include "command.h"

/**
  * A struct to hold an array of the path directories, along with the number of direcotires in the
  * path
  */
typedef struct
{
    string_t *dirs;
    size_t num_dirs;
} path_t;

status_t set_path(path_t *path, command_t *command);
status_t resize_initialize_path(path_t *path, size_t num_dirs);
void print_path(path_t *path);
void clear_path(path_t *path);

#endif
