/* pid_set.h
 *
 * Module used by the scheduler to store process information. The set entries
 * are represented by process descriptor structs. These structs consists of the
 * process ID (key), the number of the node and CPU the process is running on
 * and the class of the process. Internally, the entries are stored in a
 * red-black binary search tree. */

#ifndef _PID_SET_H
#define _PID_SET_H

#include <unistd.h>

/* Type definition for a set instance. */
typedef struct pid_set_struct pid_set_t;

/* Type definition for a process descriptor struct. */
typedef struct process_descriptor_struct process_t;

/* Process descriptor structure. */
struct process_descriptor_struct {
	pid_t pid;
	int node;
	int cpu;
	int class;
};

/* Creates a new set instance. A pointer handle to the new instance is returned
 * if the operation is successful, otherwise NULL is returned. */
pid_set_t *pid_set_create(void);

/* Destroys the specified set instance. */
void pid_set_destroy(pid_set_t *set);

/* Adds a new entry with the specified process ID, node, CPU and process
 * class to the specified set. 0 is returned on success and -1 on failure. If
 * an entry with an equal process ID already exist nothing is done and 1 is
 * returned. */
int pid_set_insert(pid_set_t *set, pid_t pid, int node, int cpu, int class);

/* Removes the entry matching the specified process ID from the specified set.
 * On success 0 is returned, otherwise -1. */
int pid_set_remove(pid_set_t *set, pid_t pid);

/* Returns the number of entries in the specified set. */
size_t pid_set_get_size(pid_set_t *set);

/* Searches the specified set for an entry matching the specified process ID.
 * On success a pointer to the process descriptor of the the entry is returned,
 * otherwise NULL. */
process_t *pid_set_get_process(pid_set_t *set, pid_t pid);

/* Returns the process descriptor of the entry with the lowest class value for
 * the specified node. If multiple entries with the same lowest class value
 * exist, the first one encountered is returned. If no entry is found NULL is
 * returned. */
process_t *pid_set_get_minimum_process(pid_set_t *set, int node);

#endif /* _PID_SET_H */
