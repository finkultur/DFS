/* pid_set.h
 *
 * Module used by the scheduler to store process information. The set entries
 * consists of the process ID (key), the number of the tile allocated to the
 * process and the class of the process.
 * Internally, the entries are represented by nodes in a red-black binary
 * search tree.
 */

#ifndef _PID_SET_H
#define _PID_SET_H

#include <unistd.h>

/* Type definition for a set instance. */
typedef struct pset_struct pid_set_t;

/* Creates a new set instance. A pointer handle to the new instance is returned
 * if the operation is successful, otherwise NULL is returned. */
pid_set_t *pid_set_create(void);

/* Destroys the specified set instance. */
void pid_set_destroy(pid_set_t *set);

/* Adds a new entry with the specified process ID, allocated tile and process
 * class to the specified set. 0 is returned on success and -1 on failure. If
 * an entry with an equal process ID already exist nothing is done and 1 is
 * returned. */
int pid_set_insert(pid_set_t *set, pid_t pid, int cpu, int class);

/* Removes the entry matching the specified process ID from the specified set.
 * On success 0 is returned, otherwise -1. */
int pid_set_remove(pid_set_t *set, pid_t pid);

/* Returns the number of entries in the specified set. */
size_t pid_set_get_size(pid_set_t *set);

/* Searches the specified set for an entry matching the specified process ID.
 * On success the number of the tile allocated to the process is returned,
 * otherwise -1. */
int pid_set_get_cpu(pid_set_t *set, pid_t pid);

/* Searches the specified set for an entry with a matching process ID. On
 * success the entry is updated with the specified tile number and 0 is
 * returned, otherwise -1. */
int pid_set_set_cpu(pid_set_t *set, pid_t pid, int cpu);

/* Searches the specified set for an entry matching the specified process ID.
 * On success the class value of the entry is returned, otherwise -1. */
int pid_set_get_class(pid_set_t *set, pid_t pid);

/* Searches the specified set for an entry matching the specified process ID.
 * On success the entry is updated with the specified process class and 0 is
 * returned, otherwise -1. */
int pid_set_set_class(pid_set_t *set, pid_t pid, int class);

/* Returns the process ID of the set entry with the lowest class value for the
 * specified tile. If multiple entries with the same lowest class value exist,
 * the first one encountered is returned. */
pid_t pid_set_get_minimum_pid(pid_set_t *set, int cpu);

/* FOR DEBUG:
 * Asserts that the set adheres to the properties of the data structure. */
int pset_assert_set(pid_set_t *set);

#endif /* _PID_SET_H */
