/* posix_test.c */

#include <stdio.h>
#include <unistd.h>

/* Test some POSIX attributes of the system. */
int main(int argc, char *argv[])
{
	// Supported POSIX version:
	long int version = sysconf(_SC_VERSION);
	// POSIX timer extension:
	long int tmr_ext = sysconf(_SC_TIMERS);
	// POSIX XSI:
	long int xsi_ext = sysconf(_XOPEN_UNIX);
	// POSIX Real Time Signals:
	long int rts_ext = sysconf(_SC_REALTIME_SIGNALS);

	if (printf("POSIX Version: %ld\n", version) == -1 ||
			printf("TMR Extension: %ld\n", tmr_ext) == -1 ||
			printf("XSI Extension: %ld\n", xsi_ext) == -1 ||
			printf("RTS Extension: %ld\n", rts_ext)  == -1)
	{
		perror("printf failure");
		return 1;
	}
	return 0;
}
