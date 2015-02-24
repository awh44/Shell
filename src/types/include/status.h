#ifndef __STATUS__H__
#define __STATUS__H__

#include <stdint.h>

/**
  * Define the error constants. These are kind of muddled, because they are a mixture of user (e.g.,
  * the user input a floating point where an integer was expected) and system (e.g., could not fork a
  * new process), but they work fine.
  */
#define SUCCESS          0
#define READ_ERROR       1
#define LINE_EMPTY       3
#define MEMORY_ERROR     4
#define FORK_ERROR       5
#define CHILD_FORK_ERR   6
#define EXEC_ERROR       7
#define NO_COMMANDS      8
#define NO_EXIST_ERROR   9
#define NUMBER_ERROR    10
#define ARGS_ERROR      11
#define CD_ERROR        12
#define EXISTS_ERROR    13
#define FORMAT_ERROR    14
#define OPEN_ERROR      15
#define ALREADY_OPEN    16
#define DUP_ERROR       17
#define DUP2_ERROR      18
#define NOT_OPEN        19
#define INVALID_VAR     20
#define DIR_ERROR       21 
#define PIPE_ERROR      22

/**
  * An error type. Returned from functions to indicate what type of error occurred; generally one of
  * the types #define'd above
  */
typedef uint64_t status_t;

/**
  * Given an error code, prints an appropriate error message, based on the above-defined constants
  * @param error_code the status_t value indicating which error occurred
  */
void error_message(status_t error_code);

#endif
