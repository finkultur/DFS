/* pid_map.h
 *
 * Module used by the scheduler to store process-to-tile mappings.
 * The map entries consists of a process ID (key) and the number of the tile
 * allocated to the process (value).
 * Internally, the entries are represented by nodes in a red-black tree to
 * achieve a time complexity of O(log(n)) for insertion, removal and lookup
 * operations.
 */

#ifndef _PID_MAP_H
#define _PID_MAP_H

#include <unistd.h>

/* Typedef for a map instance. */
typedef struct pmap_map_struct pid_map_t;

/* Creates a new map instance. A user pointer handle to the new instance is
 * returned if the operation is successful, otherwise NULL is returned. */
pid_map_t *pmap_create();

/* Destroys the specified map instance and frees up allocated memory. */
void pmap_destroy(pid_map_t *map);

/* Adds a new entry with the specified process ID and with the specified tile
 * allocation to the specified map.
 * 0 is returned on success and -1 on failure. If the entry already exist
 * nothing is done and 1 is returned. */
int pmap_insert(pid_map_t *map, pid_t pid, int cpu);

/* Removes the entry with a matching process ID from the specified map.
 * On success 0 is returned, otherwise -1. */
int pmap_remove(pid_map_t *map, pid_t pid);

/* Searches the specified map for an entry matching the specified process ID.
 * On success the number of the tile allocated to the process is returned,
 * otherwise -1. */
int pmap_get_cpu(pid_map_t *map, pid_t pid);

/* Searches the specified map for an entry with a matching process ID. On
 * success the entry is updated with the specified tile number and 0 is
 * returned, otherwise -1.  */
int pmap_set_cpu(pid_map_t *map, pid_t pid, int cpu);

/* Returns the number of entries in the specified map. */
size_t pmap_get_size(pid_map_t *map);

/* FOR DEBUG:
 * Asserts that the map adheres to the red-black tree properties. */
int pmap_assert_map_properties(pid_map_t *map);

#endif /* _PID_MAP_H */
