/* cmd_list.c
 *
 * Implementation of the command list module.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_list.h"

// Size of string buffers:
#define BUFFER_SIZE 256
// Line token delimiter:
#define DELIMITER " "

// List node struct:
struct cmd_node_struct
{
	struct cmd_node_struct *next;
	struct cmd_entry_struct *entry;
};

// List struct:
struct cmd_list_struct
{
	struct cmd_node_struct *head;
	struct cmd_node_struct *tail;
};

// Parser function, see below:
static struct cmd_entry_struct *parse_line(char *line);

// Free memory allocated for the node/entry, see below:
static void free_node(struct cmd_node_struct *node);

// Create command list:
struct cmd_list_struct *create_cmd_list(char *file_name)
{
	FILE *input_file;
	struct cmd_list_struct *list;
	struct cmd_node_struct *new_node;
	struct cmd_entry_struct *new_entry;
	char line_buf[BUFFER_SIZE];

	// Open input file:
	if ((input_file = fopen(file_name, "r")) == NULL )
	{
		return NULL ;
	}
	// Allocate and init list struct:
	if ((list = malloc(sizeof(struct cmd_list_struct))) == NULL )
	{
		return NULL ;
	}
	list->head = NULL;
	list->tail = NULL;
	// Read file line by line until EOF:
	while (fgets(line_buf, BUFFER_SIZE, input_file) != NULL )
	{
		// Parse read line:
		if ((new_entry = parse_line(line_buf)) == NULL )
		{
			return NULL ;
		}
		// Allocated and init list node:
		if ((new_node = malloc(sizeof(struct cmd_node_struct))) == NULL )
		{
			return NULL ;
		}
		new_node->next = NULL;
		new_node->entry = new_entry;
		// Insert node in list:
		if (list->head != NULL )
		{
			list->tail->next = new_node;
			list->tail = new_node;
		}
		else
		{
			list->head = new_node;
			list->tail = new_node;
		}
	}
	return list;
}

// Destroy list and free memory:
void destroy_cmd_list(struct cmd_list_struct *list)
{
	struct cmd_node_struct *node, *next_node;

	// Non-empty list:
	if (list->head != NULL )
	{
		node = list->head;
		// Free all nodes and command entries:
		while (node != NULL )
		{
			next_node = node->next;
			free_node(node);
			node = next_node;
		}
	}
	// Free list:
	free(list);
}

// Get first entry in list:
struct cmd_entry_struct *get_first(struct cmd_list_struct *list)
{
	// Return NULL if list is empty:
	if (list->head == NULL )
	{
		return NULL ;
	}
	// Return first entry:
	return list->head->entry;
}

// Remove first entry in list:
void remove_first(struct cmd_list_struct *list)
{
	struct cmd_node_struct *new_head;

	// Do nothing if list is empty:
	if (list->head == NULL )
	{
		return;
	}
	// Free head node:
	new_head = list->head->next;
	free_node(list->head);
	list->head = new_head;
	// Set tail NULL if list is now empty:
	if (list->head == NULL )
	{
		list->tail = NULL;
	}
}

// Free memory allocated to the node:
static void free_node(struct cmd_node_struct *node)
{
	int arg_index;

	free(node->entry->dir);
	free(node->entry->cmd);
	// Free all arguments from argv:
	arg_index = 0;
	while (node->entry->argv[arg_index] != NULL )
	{
		free(node->entry->argv[arg_index]);
		arg_index++;
	}
	free(node->entry->argv);
	free(node->entry);
	free(node);
}

// Parse contents of the specified line and create command entry:
static struct cmd_entry_struct *parse_line(char *line)
{
	int token_count, arg_count, arg_index, str_length;
	char *token, *line_break;
	char parse_buf[BUFFER_SIZE];
	struct cmd_entry_struct *new_entry;

	// Copy line to parse buffer:
	strcpy(parse_buf, line);
	// Count number of tokens:
	if (strtok(parse_buf, DELIMITER) != NULL )
	{
		token_count = 1;
	}
	else
	{
		return NULL ;
	}
	while (strtok(NULL, DELIMITER) != NULL )
	{
		token_count++;
	}
	// A properly formatted line has at least 3 tokens:
	if (token_count < 3)
	{
		return NULL ;
	}
	// Allocate entry struct:
	if ((new_entry = malloc(sizeof(struct cmd_entry_struct))) == NULL )
	{
		return NULL ;
	}
	// Copy line to parse buffer:
	strcpy(parse_buf, line);
	// Find and replace line break (if present):
	if ((line_break = strchr(parse_buf, '\n')) != NULL )
	{
		*line_break = '\0';
	}
	// Parse start time:
	token = strtok(parse_buf, DELIMITER);
	new_entry->start_time = atoi(token);
	// Parse working dir:
	token = strtok(NULL, DELIMITER);
	str_length = strlen(token) + 1;
	if ((new_entry->dir = malloc(sizeof(char) * str_length)) == NULL )
	{
		return NULL ;
	}
	strcpy(new_entry->dir, token);
	// Parse command:
	token = strtok(NULL, DELIMITER);
	str_length = strlen(token) + 1;
	if ((new_entry->cmd = malloc(sizeof(char) * str_length)) == NULL )
	{
		return NULL ;
	}
	strcpy(new_entry->cmd, token);
	// Parse argv:
	arg_count = token_count - 1;
	if ((new_entry->argv = malloc(sizeof(char*) * arg_count)) == NULL )
	{
		return NULL ;
	}
	// argv[0] = command:
	if ((new_entry->argv[0] = malloc(sizeof(char) * str_length)) == NULL )
	{
		return NULL ;
	}
	strcpy(new_entry->argv[0], token);
	// Parse remaining arguments (if any):
	arg_index = 1;
	while ((token = strtok(NULL, DELIMITER)) != NULL )
	{
		str_length = strlen(token) + 1;
		if ((new_entry->argv[arg_index] = malloc(sizeof(char) * str_length))
				== NULL )
		{
			return NULL ;
		}
		strcpy(new_entry->argv[arg_index], token);
		arg_index++;
	}
	// Set NULL-pointer at last index of argv:
	new_entry->argv[arg_index] = NULL;
	// Return entry:
	return new_entry;
}
