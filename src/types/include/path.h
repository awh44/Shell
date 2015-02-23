#ifndef __PATH__H__
#define __PATH__H__

#include "command.h"
#include "status.h"
#include "string_t.h"

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
  * Sets the given path appropriately, given a command from which it should be set
  * @param path    the path which is to be set
  * @param command the command from which the path is being set
  * @return a status code indicating whether an error occurred during execution of the function
  */
status_t set_path(path_t *path, command_t *command);

/**
  * Resizes the path to hold num_dirs number of directories and initializes all of the string_t
  * variables in the new array
  * @param path     the path variable to resized
  * @param num_dirs the number of directories the path is to hold
  */
status_t resize_initialize_path(path_t *path, size_t num_dirs);

/**
  * Prints the path variable, one directory per line
  * @param path the path to be printed
  */
void print_path(path_t *path);

/**
  * Clears and frees all of the memory associated with the path
  * @param path the path to be cleared
  */
void clear_path(path_t *path);

#endif
