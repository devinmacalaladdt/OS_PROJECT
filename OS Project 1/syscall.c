/* syscall.c
 *
 * Group Members Names and NetIDs:
 *   1. Devin Macalalad (dtm97)
 *   2. David Gasperini (dlg195)
 *
 * ILab Machine Tested on:
 * grep.cs.rutgers.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>                                                                                
#include <sys/syscall.h>

double avg_time = 0;

int main(int argc, char *argv[]) {

    /* Implement Code Here */
	struct timeval start;
	gettimeofday(&start, NULL);
	int x;
	for (x = 0; x < 5000000; x++) {
		syscall(SYS_getuid);//no need to store result because it's just a time test
	}
	struct timeval end;
	gettimeofday(&end, NULL);
	avg_time = (-(start.tv_sec + start.tv_usec) + (end.tv_sec + end.tv_usec)) / (5000000.0);
    // Remember to place your final calculated average time
    // per system call into the avg_time variable

    printf("Average time per system call is %f microseconds\n", avg_time);

    return 0;
}
