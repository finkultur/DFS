/* cmd_queue_test.c
 *
 * Simple test program for the commad queue module.
 * */

#include "cmd_queue.h"
#include <stdio.h>

// Test command queue module:
int main(int argc, char *argv[])
{
	int arg_index;
	cmd_queue_t *queue;
	cmd_t *cmd;
	// Check command line argument:
	if (argc != 2) {
		printf("usage: %s <workload file>\n", argv[0]);
		return 1;
	}
	// Create queue and check for error:
	printf("creating queue...");
	fflush(stdout);
	if ((queue = cmd_queue_create(argv[1])) == NULL) {
		printf("failed to create command queue from file %s\n", argv[1]);
		return 1;
	}
	printf("done\n");
	// Read and print each command:
	printf("parsed commands:\n");
	while (cmd_queue_get_size(queue) > 0) {
		cmd = cmd_queue_front(queue);
		printf("[");
		printf("start: %i ", cmd->start);
		printf("class: %i ", cmd->class);
		printf("dir: %s ", cmd->dir);
		printf("cmd: %s ", cmd->cmd);
		printf("stdin: %s ", cmd->input_file);
		printf("stdout: %s ", cmd->output_file);
		arg_index = 0;
		while (cmd->argv[arg_index] != NULL) {
			printf("argv[%i]: %s ", arg_index, cmd->argv[arg_index]);
			arg_index++;
		}
		printf("]\n\n");
		cmd_queue_dequeue(queue);
	}
	printf("destroying list...");
	fflush(stdout);
	cmd_queue_destroy(queue);
	printf("done\n");
	return 0;
}
