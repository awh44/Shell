#ifndef __STATUS__H__
#define __STATUS__H__

#include <stdint.h>

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

/**
  * An error type. Returned from functions to indicate what type of error occurred; generally one of
  * the types #define'd above
  */
typedef uint64_t status_t;


void error_message(status_t error_code);
#endif
