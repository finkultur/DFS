/* proc_table_test.c
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "proc_table.h"

int main(int argc, char *argv[])
{
	int i, pid, tile;

	fprintf(stderr, "creating table...");
	proc_table table = create_proc_table(64);
	if (table == NULL )
	{
		fprintf(stderr, "failed to create table\n");
		return 1;
	}
	fprintf(stderr, "done\n");

	srand(time(NULL ));

	for (i = 0; i < 100; i++)
	{
		pid = rand();
		tile = rand() % 64;
		if (add_pid(table, pid, tile) == 0)
		{
			fprintf(stderr, "added pid: %i on tile: %i\n", pid, tile);
		}
		else
		{
			fprintf(stderr, "failed to add pid: %i on tile: %i+n", pid, tile);
		}
	}

	fprintf(stderr, "destroying table...");
	destroy_proc_table(table);
	fprintf(stderr, "done\n");

	return 0;
}
