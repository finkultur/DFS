/* cmd_list.h
 *
 * A command list module for reading and parsing the workload input file. A
 * linked list of all commands that should be executed is created from the
 * contents of the input file.
 *
 * Each line of the input file should have the format below:
 * <START> <DIRECTORY> <COMMAND>
 * */

#ifndef _CMD_LIST_H
#define _CMD_LIST_H

/* Struct representing a linked list of commands. */
struct cmd_list_struct;

/* Struct representing each entry (command) in the list. */
struct cmd_entry_struct
{
	int start_time;  // Command start time
	int class; // Class number, 0 for undefined
	char *dir;  // Working directory
	char *cmd;  // Command name
	char **argv;    // Argument vector
    char *new_stdin;    // Redirect stdin?
    char *new_stdout;   // Redirect stdout?
};

/* Type definition of a command list. */
typedef struct cmd_list_struct *cmd_list;

/* Type definition of a command list entry. */
typedef struct cmd_entry_struct *cmd_entry;

/* Creates a new command list by reading and parsing the contents of the file
 * with the specified file name. Returns a pointer to the created list on
 * success, otherwise NULL. */
cmd_list create_cmd_list(char *file_name);

/* Destroys the specified list and frees allocated memory for all remaining
 * entries. */
void destroy_cmd_list(cmd_list list);

/* Returns a pointer to the first entry in the list without removing it.
 * If the list is empty a NULL-pointer is returned. */
cmd_entry get_first(cmd_list list);

/* Removes the first command entry in the list and frees allocated memory. */
void remove_first(cmd_list list);

#endif
