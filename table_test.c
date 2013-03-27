/* table_test.c */

#include "pid_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const int table_size = 8;
static const int num_entries = 8;

int main(int argc, char *argv[])
{
	int n, pid, tile_num;
	pid_table table;

	srand(time(NULL));

	fputs("creating table...", stderr);
	table = create_table(table_size);
	fputs("ok\n", stderr);

	fprintf(stderr, "table size: %i\n", get_size(table));

	fprintf(stderr, "table count: %i\n", get_count(table));

	for (n = 0; n < num_entries; n++)
	{
		pid = rand();
		tile_num = rand() % 64;
		fprintf(stderr, "adding pid: %i tile_num: %i) ", pid, tile_num),
		add_pid(table, pid, tile_num);
		fputs("[ok]\n", stderr);
		fprintf(stderr, "table count: %i\n", get_count(table));
	}

	fputs("printing table:\n", stderr);
	print_table(table);

	fputs("destroying table...", stderr);
	destroy_table(table);
	fputs("ok\n", stderr);

	return 0;
}
