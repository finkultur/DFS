/* cmd_queue.c
 *
 * Implementation of the command queue module.
 * */

#include "cmd_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Size of string buffers. */
#define BUFFER_SIZE 256
/* Line token delimiter. */
#define DELIMITER " "
/* Minimum number of tokens contained in a properly formatted command line. */
#define MIN_TOKENS 4
/* Minimum length command argument vector. */
#define MIN_ARGV_LENGTH 2

/* Typedef for a queue entry. */
typedef struct cmd_queue_entry_struct entry_t;

/* Queue entry type (linked list node). */
struct cmd_queue_entry_struct {
	entry_t *next;
	cmd_t cmd_desc;
};

/* Queue type. */
struct cmd_queue_struct {
	size_t size;
	entry_t *front;
	entry_t *back;
};

/* Helper functions, see definitions below. */
static int read_lines(char *filename, cmd_queue_t *queue);
static int parse_line(char *line, entry_t *entry);
static void enqueue_entry(cmd_queue_t *queue, entry_t *entry);
static void free_entry(entry_t *entry);

/* Creates a command queue from the specified workload. */
cmd_queue_t *cmd_queue_create(char *filename)
{
	cmd_queue_t *queue;
	/* Allocate and initialize a new queue. */
	if ((queue = calloc(1, sizeof(cmd_queue_t))) != NULL) {
		read_lines(filename, queue);
	}
	return queue;
}

/* Destroys queue and deallocates memory. */
void cmd_queue_destroy(cmd_queue_t *queue)
{
	/* Free all command entries. */
	while (queue->size > 0) {
		cmd_queue_dequeue(queue);
	}
	free(queue);
}

/* Gets the descriptor for the first command in the queue. */
cmd_t *cmd_queue_front(cmd_queue_t *queue)
{
	/* Return NULL if queue is empty. */
	if (queue->size == 0) {
		return NULL;
	} else {
		return &(queue->front->cmd_desc);
	}
}

/* Removes the first entry in the queue and deallocates memory. */
void cmd_queue_dequeue(cmd_queue_t *queue)
{
	entry_t *entry;
	/* Nothing to do if queue is empty. */
	if (queue->size != 0) {
		entry = queue->front;
		queue->front = queue->front->next;
		free_entry(entry);
		queue->size--;
		if (queue->size == 0) {
			queue->back = NULL;
		}
	}
}

/* Gets the size of the command queue. */
size_t cmd_queue_get_size(cmd_queue_t *queue)
{
	return queue->size;
}

/* Reads commands from the specified workload file and enqueues them. On success
 * the number of commands read is returned, otherwise -1. */
static int read_lines(char *filename, cmd_queue_t *queue)
{
	FILE *input;
	entry_t *entry;
	char buffer[BUFFER_SIZE];
	/* Open input file. */
	if ((input = fopen(filename, "r")) == NULL) {
		return -1;
	}
	/* Read file line by line until EOF. */
	while (fgets(buffer, BUFFER_SIZE, input) != NULL) {
		/* Skip if line is empty or a comment. */
		if (buffer[0] == '#' || buffer[0] == '\n') {
			continue;
		}
		/* Allocate new entry. */
		if ((entry = calloc(1, sizeof(entry_t))) == NULL) {
			break;
		}
		/* Parse read line, break on failure. */
		if (parse_line(buffer, entry) == 0) {
			enqueue_entry(queue, entry);
		} else {
			free_entry(entry);
			break;
		}
	}
    fclose(input);
	return queue->size;
}

/* Parse contents of line and fill in the command descriptor for the entry. */
static int parse_line(char *line, entry_t *entry)
{
	int num_token, token_length, argv_length, arg_pos;
	char *token, *line_break;
	char buffer[BUFFER_SIZE];
	/* Count number of tokens. */
	num_token = 0;
	strncpy(buffer, line, BUFFER_SIZE);
	if (strtok(buffer, DELIMITER) != NULL) {
		num_token++;
		while (strtok(NULL, DELIMITER) != NULL) {
			num_token++;
		}
	}
	if (num_token < MIN_TOKENS) {
		return -1;
	}
	/* Find and replace line break. */
	strncpy(buffer, line, BUFFER_SIZE);
	if ((line_break = strchr(buffer, '\n')) != NULL) {
		*line_break = '\0';
	}
	/* Parse start time. */
	if ((token = strtok(buffer, DELIMITER)) == NULL) {
		return -1;
	}
	entry->cmd_desc.start = atoi(token);
	/* Parse class. */
	if ((token = strtok(NULL, DELIMITER)) == NULL) {
		return -1;
	}
	entry->cmd_desc.class = atoi(token);
	/* Parse working directory. */
	if ((token = strtok(NULL, DELIMITER)) == NULL) {
		return -1;
	}
	token_length = strlen(token) + 1;
	if ((entry->cmd_desc.dir = malloc(token_length * sizeof(char))) == NULL) {
		return -1;
	}
	strncpy(entry->cmd_desc.dir, token, token_length);
	/* Parse command. */
	if ((token = strtok(NULL, DELIMITER)) == NULL) {
		return -1;
	}
	token_length = strlen(token) + 1;
	if ((entry->cmd_desc.cmd = malloc(token_length * sizeof(char))) == NULL) {
		return -1;
	}
	strncpy(entry->cmd_desc.cmd, token, token_length);
	/* Calculate required length of argument vector. */
	argv_length = num_token - MIN_TOKENS + 2;
	/* Check if command contains stdin/stdout redirection. */
	if (strchr(line, '<') != NULL) {
		argv_length = argv_length - 2;
	}
	if (strchr(line, '>')) {
		argv_length = argv_length - 2;
	}
	/* Check that the argument vector length is sound. */
	if (argv_length < MIN_ARGV_LENGTH) {
		return -1;
	}
	/* Allocate argument vector. */
	if ((entry->cmd_desc.argv = calloc(argv_length, sizeof(char *))) == NULL) {
		return -1;
	}
	/* Set argv[0] = command. */
	if ((entry->cmd_desc.argv[0] = malloc(token_length * sizeof(char))) == NULL) {
		return -1;
	}
	strncpy(entry->cmd_desc.argv[0], token, token_length);
	/* Parse any remaining arguments. */
	arg_pos = 1;
	while ((token = strtok(NULL, DELIMITER)) != NULL) {
		/* Check for stdin/stdout redirection. */
		if (token[0] == '<') {
			token = strtok(NULL, DELIMITER);
			if (token == NULL) {
				return -1;
			}
			token_length = strlen(token) + 1;
			if ((entry->cmd_desc.input_file = malloc(token_length
					* sizeof(char))) == NULL) {
				return -1;
			}
			strncpy(entry->cmd_desc.input_file, token, token_length);
		} else if (token[0] == '>') {
			token = strtok(NULL, DELIMITER);
			if (token == NULL) {
				return -1;
			}
			token_length = strlen(token) + 1;
			if ((entry->cmd_desc.output_file = malloc(token_length
					* sizeof(char))) == NULL) {
				return -1;
			}
			strncpy(entry->cmd_desc.output_file, token, token_length);
		} else {
			token_length = strlen(token) + 1;
			if ((entry->cmd_desc.argv[arg_pos] = malloc(token_length
					* sizeof(char))) == NULL) {
				return -1;
			}
			strncpy(entry->cmd_desc.argv[arg_pos], token, token_length);
			arg_pos++;
		}
	}
	return 0;
}

/* Inserts the specified entry at the back of the specified queue. */
static void enqueue_entry(cmd_queue_t *queue, entry_t *entry)
{
	/* Insert entry. */
	if (queue->size == 0) {
		queue->front = entry;
		queue->back = entry;
	} else {
		queue->back->next = entry;
		queue->back = entry;
	}
	queue->size++;
}

/* Free memory allocated by the entry. */
static void free_entry(entry_t *entry)
{
	int index;
	free(entry->cmd_desc.dir);
	free(entry->cmd_desc.cmd);
	free(entry->cmd_desc.input_file);
	free(entry->cmd_desc.output_file);
	/* Free all arguments from argv. */
	if (entry->cmd_desc.argv != NULL) {
		for (index = 0; entry->cmd_desc.argv[index] != NULL; index++) {
			free(entry->cmd_desc.argv[index]);
		}
	}
	free(entry);
}
