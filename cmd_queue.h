/* cmd_queue.h
 *
 * A command queue module for reading and parsing the workload input file. A
 * queue of all commands that should be executed is created from the contents
 * of the input file. Each command is represented by a descriptor struct that
 * contains the information needed to execute the program. Internally, the
 * queue is implemented as a linked list where each entry in the list holds a
 * command descriptor.
 *
 * Each line of the workload file should have the following format:
 * <START TIME> <PROCESS CLASS> <WORKING DIRECTORY> <COMMAND>
 *
 * The queue if FIFO, i.e. commands will be returned in the order they were
 * listed in the input file, from top to bottom. Empty lines and lines
 * beginning with a '#' are ignored by the parser to allow the input file to
 * contain comments.
 * */

#ifndef _CMD_QUEUE_H
#define _CMD_QUEUE_H

#include <stdlib.h>

/* Type definition of a command queue. */
typedef struct cmd_queue_struct cmd_queue_t;

/* Type definition of a command description. */
typedef struct cmd_descriptor_struct cmd_t;

/* Type describing a command. */
struct cmd_descriptor_struct {
	int start; // Command start time
	int class; // Class number, 0 for undefined
	char *dir; // Working directory
	char *cmd; // Command
	char *input_file; // Redirect stdin
	char *output_file; // Redirect stdout
	char **argv; // Argument vector
};

/* Creates a new command queue by reading and parsing the contents of the file
 * with the specified file name. Returns a pointer to the created queue on
 * success, otherwise NULL. */
cmd_queue_t *cmd_queue_create(char *filename);

/* Destroys the specified queue. */
void cmd_queue_destroy(cmd_queue_t *queue);

/* Returns a pointer to the command descriptor of the first command in the
 * queue. If the queue is empty a NULL-pointer is returned. */
cmd_t *cmd_queue_front(cmd_queue_t *queue);

/* Removes the first command in the queue. */
void cmd_queue_dequeue(cmd_queue_t *queue);

/* Returns the number of commands in the queue. */
size_t cmd_queue_get_size(cmd_queue_t *queue);

#endif /* _CMD_QUEUE_H */
