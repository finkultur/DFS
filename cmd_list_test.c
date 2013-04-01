/* cmd_list_test.c
 *
 * Simple test program for the commad list module.
 * */

#include <stdio.h>

#include "cmd_list.h"

// Test command list module:
int main(int argc, char *argv[])
{
	int arg_index;
	cmd_list list;
	cmd_entry cmd;

	// Check command line argument:
	if (argc != 2)
	{
		printf("usage: %s <workload file>\n", argv[0]);
		return 1;
	}
	// Create list and check for error:
	printf("creating list...");
	if ((list = create_cmd_list(argv[1])) == NULL )
	{
		printf("failed to create command list from file %s\n", argv[1]);
		return 1;
	}
	printf("done\n");
	// Read and print each command:
	while ((cmd = get_first(list)) != NULL )
	{
		printf("start: %i ", cmd->start);
		printf("dir: %s ", cmd->dir);
		printf("cmd: %s ", cmd->cmd);
		arg_index = 0;
		while (cmd->argv[arg_index] != NULL )
		{
			printf("argv[%i]: %s ", arg_index, cmd->argv[arg_index]);
			arg_index++;
		}
		printf("\n");
		remove_first(list);
	}
	printf("destroying list...");
	destroy_cmd_list(list);
	printf("done\n");
	return 0;
}
