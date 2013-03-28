/* pid_table.c
 *
 * Implementation of the pid_table module. 
 */

#include <stdio.h>
#include <stdlib.h>

#include "pid_table.h"

// Table struct:
struct pid_table_struct
{
    struct pid_entry_struct **data;
    int size;
    int count;
};

// Table entry struct:
struct pid_entry_struct
{
    pid_t pid;
    int tile_num;
    struct pid_entry_struct *next;
    struct pid_entry_struct *prev;
};

// Get the hashed table index for the specified pid:
int hash_value(pid_table table, pid_t pid)
{
    return pid % table->size;
}

// Create new table:
pid_table create_table(int size)
{
    int index;
    struct pid_table_struct *new_table;
    if (size <= 0)
    {
        return NULL ;
    }
    // Allocate table struct, return on error:
    new_table = malloc(sizeof(struct pid_table_struct));
    if (new_table == NULL )
    {
        return NULL ;
    }
    // Allocate data array (entry pointers), return on error:
    new_table->data = malloc(size * sizeof(struct pid_entry_struct*));
    if (new_table->data == NULL )
    {
        free(new_table);
        return NULL ;
    }
    new_table->size = size;
    new_table->count = 0;
    // Init data array with NULL-pointers:
    for (index = 0; index < size; index++)
    {
        new_table->data[index] = NULL;
    }
    return new_table;
}

// Deallocate table and all entries:
void destroy_table(pid_table table)
{
    int index;
    struct pid_entry_struct *entry_itr, *next_itr;
    if (table == NULL )
    {
        return;
    }
    if (table->count == 0) // Table empty?
    {
        free(table->data);
        free(table);
    }
    else
    {
        // Free entries at all tabled indices:
        for (index = 0; index < table->size; index++)
        {
            if (table->data[index] != NULL )
            {
                entry_itr = table->data[index];
                while (entry_itr != NULL )
                {
                    next_itr = entry_itr->next;
                    free(entry_itr);
                    entry_itr = next_itr;
                }
            }
        }
        free(table->data);
        free(table);
    }
}

// Return table size:
int get_size(pid_table table)
{
    if (table != NULL )
    {
        return table->size;
    }
    return -1;
}

// Return table count:
int get_count(pid_table table)
{
    if (table != NULL )
    {
        return table->count;
    }
    return -1;
}

// Add new pid to table:
int add_pid(pid_table table, pid_t pid, int tile_num)
{
    int hash_index;
    struct pid_entry_struct *new_entry, *entry_itr;
    // Allocate entry struct:
    new_entry = malloc(sizeof(struct pid_entry_struct));
    if (new_entry == NULL )
    {
        return -1;
    }
    // Init entry:
    new_entry->pid = pid;
    new_entry->tile_num = tile_num;
    new_entry->next = NULL;
    new_entry->prev = NULL;
    // Get table index for entry:
    hash_index = hash_value(table, pid);
    // Table empty at index?
    if (table->data[hash_index] == NULL )
    {
        table->data[hash_index] = new_entry;
    }
    else
    {
        // Find last entry at index and insert new entry:
        entry_itr = table->data[hash_index];
        while (entry_itr->next != NULL )
        {
            entry_itr = entry_itr->next;
        }
        entry_itr->next = new_entry;
        new_entry->prev = entry_itr;
    }
    table->count++;
    return 0;
}

// Remove pid:
int remove_pid(pid_table table, pid_t pid)
{
    int hash_index;
    struct pid_entry_struct *entry_itr;
    // Get hashed table index:
    hash_index = hash_value(table, pid);
    // Table empty at index?
    if (table->data[hash_index] == NULL )
    {
        return -1;
    }
    // Init iterator:
    entry_itr = table->data[hash_index];
    // Search for an entry with equal pid in the list at hashed index:
    while (entry_itr->pid != pid)
    {
        entry_itr = entry_itr->next;
        // Return if end of list reached:
        if (entry_itr == NULL )
        {
            return -1;
        }
    }
    // First entry in list at index:
    if (entry_itr->prev == NULL )
    {
        table->data[hash_index] = entry_itr->next;
        if (entry_itr->next != NULL )
        {
            entry_itr->next->prev = NULL;
        }
    }
    else
    {
        entry_itr->prev->next = entry_itr->next;
        // If not last entry in list:
        if (entry_itr->next != NULL )
        {
            entry_itr->next->prev = entry_itr->prev;
        }
    }
    // Deallocate entry:
    free(entry_itr);
    table->count--;
    return 0;
}

// Get tile_num for pid:
int get_tile_num(pid_table table, pid_t pid)
{
    int hash_index;
    struct pid_entry_struct *entry_itr;
    // Get hashed table index:
    hash_index = hash_value(table, pid);
    // Search list at table index for a matching entry:
    entry_itr = table->data[hash_index];
    // Loop to end of list:
    while (entry_itr != NULL )
    {
        // Return if mathching entry found:
        if (entry_itr->pid == pid)
        {
            return entry_itr->tile_num;
        }
        entry_itr = entry_itr->next;
    }
    // Pid not found:
    return -1;
}

// Set tile_num for specified pid:
int set_tile_num(pid_table table, pid_t pid, int tile_num)
{
    int hash_index;
    struct pid_entry_struct *entry_itr;
    // Get hashed table index:
    hash_index = hash_value(table, pid);
    // Search list at table index for a matching entry:
    entry_itr = table->data[hash_index];
    while (entry_itr != NULL)
    {
        // Update tile_num and return if matching entry is found:
        if (entry_itr->pid == pid)
        {
            entry_itr->tile_num = tile_num;
            return 0;
        }
        entry_itr = entry_itr->next;
    }
    // Pid not found:
    return -1;
}

// TESTING
// Prints table to standard output:
void print_table(pid_table table)
{
    int n;
    struct pid_entry_struct *itr;

    printf("Printing table:\n");

    for (n = 0; n < table->size; n++)
    {
        if (table->data[n] == NULL )
        {
            printf("table index %i: NULL\n", n);
        }
        else
        {
            printf("table index %i: ", n);
            itr = table->data[n];
            while (itr != NULL )
            {
                if (itr->next != NULL )
                {
                    printf("(pid: %i , tile_num: %i) -> ", itr->pid,
                            itr->tile_num);
                }
                else
                {
                    printf("(pid: %i , tile_num: %i)\n", itr->pid,
                            itr->tile_num);
                }
                itr = itr->next;
            }
        }
    }
}
