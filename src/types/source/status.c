#include <stdio.h>

#include "../include/status.h"

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
		case EXISTS_ERROR:
			fprintf(stderr, "Error: That alias already exists.");
			break;
		case FORMAT_ERROR:
			fprintf(stderr, "Error: Incorrect format.");
			break;
		case OPEN_ERROR:
			fprintf(stderr, "Error: Could not open file.");
			break;
		case ALREADY_OPEN:
			fprintf(stderr, "Error: Script file already open.");
			break;
		case DUP_ERROR:
			fprintf(stderr, "Error: Could not map file to stdout.");
			break;
		case DUP2_ERROR:
			fprintf(stderr, "Error: Could not stdout or stderr to file.");
			break;
		case NOT_OPEN:
			fprintf(stderr, "Error: No script file currently open.");
			break;
		case INVALID_VAR:
			fprintf(stderr, "Error: That value cannot currently be set.");
			break;
		case DIR_ERROR:
			fprintf(stderr, "Error: Could not add directory to path: ");
			break;
		default:
			fprintf(stderr, "Error: Unknown error.");
	}
	fprintf(stderr, "\n");
}

