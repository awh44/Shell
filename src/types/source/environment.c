#include <unistd.h>

#include "../include/environment.h"

void clear_environment(environment_t *environment)
{
	clear_path(environment->path);
	clear_history(environment->history);
	clear_aliases(environment->aliases);
	if (environment->script_file >= 0)
	{
		close(environment->script_file);
	}
	environment->script_file = -1;
	string_uninitialize(environment->prompt);
}
